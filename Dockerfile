FROM ubuntu:24.04

RUN DEBIAN_FRONTEND=noninteractive apt-get update && apt-get upgrade -y && \
    apt-get install -y \
    cmake \
    make \
    gcc \
    g++ \
    pkg-config \
    libzmq3-dev \
    iproute2 \
    uhd-host \
    libgtest-dev \
    iperf3 \
    libfftw3-dev \
    libmbedtls-dev \
    libsctp-dev \
    libyaml-cpp-dev \
    net-tools \
    libboost-all-dev \
    libconfig++-dev \
    libxcb-cursor0 \
    libgles2-mesa-dev \
    gr-osmosdr \
    libuhd-dev

WORKDIR /app

COPY . .

RUN rm -rf build && mkdir -p build && cd build && cmake -DENABLE_ZEROMQ=ON .. && make -j$(nproc) && make install && ldconfig
# RUN rm -rf build && mkdir -p build && cd build && \
#     cmake -DENABLE_ZEROMQ=ON \
#           -DCPU_DISPATCH=OFF \
#           -DENABLE_DISPATCH=OFF \
#           -DENABLE_AVX2=OFF \
#           -DENABLE_AVX512=OFF \
#           -DPORTABLE_BUILD=ON \
#           -DCMAKE_C_FLAGS_RELEASE="-O3 -pipe -march=x86-64 -mtune=generic" \
#           -DCMAKE_CXX_FLAGS_RELEASE="-O3 -pipe -march=x86-64 -mtune=generic" \
#           .. && \
#     make -j$(nproc) && make install && ldconfig

#this is where you change how itll be run, find the larger project he added it to ///wherever they integrated everything and 

# CMD ["prach-agent", "--config", "/prach.toml"]
CMD ["prach-agent", "--config", "/app/configs/basic_prach.yaml"]
#CMD [ "sh", "-c", "/usr/local/bin/jammer --config /jammer.yaml $ARGS" ]