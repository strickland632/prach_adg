# ---------- Stage 1: build ----------
FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

SHELL ["/bin/bash", "-o", "pipefail", "-c"]


# Toolchain + libraries your CMakeLists.txt expects
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake pkg-config git ccache \
    libfftw3-dev libmbedtls-dev libliquid-dev \
    libzmq3-dev libuhd-dev uhd-host \
    libboost-program-options-dev libboost-system-dev \
    libsctp-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
COPY . .

# Configure (keep RF features ON so libsrsran_rf builds and links)
# Add toggles here if you need to force options: -DENABLE_UHD=ON -DENABLE_ZEROMQ=ON
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

# Build (single job + verbose to surface exact failures)
RUN cmake --build build -j1 -- VERBOSE=1

# Install to a clean prefix
RUN cmake --install build --prefix /opt/prach-agent

# If UHD images exist, copy them for the runtime (harmless otherwise)
RUN if [ -d /usr/share/uhd ]; then \
      mkdir -p /opt/prach-agent/uhd-share && \
      cp -a /usr/share/uhd /opt/prach-agent/uhd-share/; \
    fi

# Bundle all runtime .so deps actually required by installed binaries
RUN set -euo pipefail; \
    mkdir -p /opt/prach-agent/opt-libs; \
    find /opt/prach-agent/bin -type f -executable | while read -r bin; do \
      ldd "$bin" | awk '/=> \// {print $3} /^\/lib/ {print $1}' || true; \
    done | sort -u > /tmp/needed.libs; \
    # Skip core libs the base image already has
    grep -vE '/(ld-linux|libc\.so|libm\.so|librt\.so|libpthread\.so|libgcc_s\.so|libstdc\+\+\.so)' /tmp/needed.libs \
      | xargs -r -I{} cp --parents -v {} /opt/prach-agent/opt-libs/; \
    echo "== Bundled libs ==" && cat /tmp/needed.libs || true


# ---------- Stage 2: runtime ----------
FROM ubuntu:22.04 AS runtime
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Non-root user
RUN useradd -m -u 10001 appuser
WORKDIR /app

# App + bundled libs + (optional) UHD images
COPY --from=builder /opt/prach-agent /opt/prach-agent

# Make sure bundled libs are found
ENV PATH="/opt/prach-agent/bin:${PATH}"
ENV LD_LIBRARY_PATH="/opt/prach-agent/opt-libs/lib:/opt/prach-agent/opt-libs/usr/lib:/opt/prach-agent/opt-libs/usr/lib/x86_64-linux-gnu:/opt/prach-agent/opt-libs/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH:-}"

# Point UHD to images if present
ENV UHD_IMAGES_DIR="/opt/prach-agent/uhd-share/uhd/images"

USER appuser

# Adjust this if your installed binary has a different name/path
ENTRYPOINT ["/opt/prach-agent/bin/prach-agent"]
# CMD ["--help"]
