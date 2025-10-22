// prach_tx.cc
#define _GLIBCXX_USE_CXX11_ABI 0
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <complex>
#include <string>
#include <sys/time.h>



extern "C" {
#include "srsran/srsran.h"
}

#include "args.h"
#include "source.h"   // <-- device

#define MAX_LEN 70176

// ---------- tiny time helper ----------
static inline unsigned long long us_since(const timeval& a, const timeval& b)
{
  return (unsigned long long)(b.tv_sec - a.tv_sec) * 1000000ULL +
         (unsigned long long)(b.tv_usec - a.tv_usec);
}

// ---------- struct to send burst
struct PrachBurst {
  const cf_t* samples        = nullptr;  // pointer to CP+sequence samples
  size_t      nsamps         = 0;        // total samples to send (CP + sequence)
  double      tx_rate_hz     = 0.0;
  double      center_freq_hz = 0.0;
  double      tx_gain_db     = 0.0;
  double      start_in_s     = 0.050;    // schedule a bit in the future

  bool valid() const {
    return samples != nullptr && nsamps > 0 &&
           tx_rate_hz > 0.0 && center_freq_hz > 0.0;
  }
};

// ---------- EXPECTED source.h API (adjust names if your header differs) ----------
typedef struct rf_source rf_source_t;
rf_source_t* create(YAML::Node rf_config);
// int          recv(cf_t_1* buffer, size_t nof_samples);
// int          send(cf_t_1* buffer, size_t nof_samples);
// int          rf_send_cf32_burst(rf_source_t* h, const void* samples, size_t nsamps, double start_in_s);
// void         rf_close(rf_source_t* h);

// ---------- minimal TX wrapper using source.h ----------
static void tx_send_prach(rf_source_t* rf, const PrachBurst& burst)
{
  if (!burst.valid()) {
    throw std::invalid_argument("PrachBurst is not valid");
  }

  if (rf_cfg_tx(rf, burst.tx_rate_hz, burst.center_freq_hz, burst.tx_gain_db) != 0) {
    throw std::runtime_error("rf_cfg_tx() failed");
  }

  // schedule SoB
  if (rf_send_cf32_burst(rf, (const void*)burst.samples, burst.nsamps, burst.start_in_s) != 0) {
    throw std::runtime_error("rf_send_cf32_burst() failed");
  }
}

int main(int argc, char** argv)
{
  // ---- read prach config 
  std::string config_file = "basic_prach.yaml";
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
      config_file = argv[++i];
      break;
    }
  }
  if (config_file.empty()) {
    std::fprintf(stderr, "Usage: %s --config <basic_prach.yaml>\n", argv[0]);
    return EXIT_FAILURE;
  }

  // ---- Load config ----
  all_args_t args;
  try {
    args = parseConfig(config_file);
  } catch (const std::exception& e) {
    std::fprintf(stderr, "Config error: %s\n", e.what());
    return EXIT_FAILURE;
  }

  // ---- PRACH cfg (srsRAN) ----
  srsran_prach_t     prach = {};
  srsran_prach_cfg_t prach_cfg;
  ZERO_OBJECT(prach_cfg);

  prach_cfg.is_nr            = args.is_nr;
  prach_cfg.config_idx       = args.config_idx;
  prach_cfg.hs_flag          = false;
  prach_cfg.freq_offset      = args.freq_offset;     // from YAML
  prach_cfg.root_seq_idx     = args.root_seq_idx;
  prach_cfg.zero_corr_zone   = args.zero_corr_zone;
  prach_cfg.num_ra_preambles = args.num_ra_preambles;

  // PRACH buffer (cf_t is srsRAN complex float)
  cf_t preamble[MAX_LEN];
  std::memset(preamble, 0, sizeof(preamble));

  // Init srsRAN PRACH (symbol size from PRB count)
  if (srsran_prach_init(&prach, srsran_symbol_sz(args.nof_prb)) != SRSRAN_SUCCESS) {
    std::fprintf(stderr, "Failed to init PRACH object\n");
    return EXIT_FAILURE;
  }

  timeval t0{}, t1{};
  gettimeofday(&t0, nullptr);
  if (srsran_prach_set_cfg(&prach, &prach_cfg, args.nof_prb) != SRSRAN_SUCCESS) {
    std::fprintf(stderr, "Error configuring PRACH\n");
    srsran_prach_free(&prach);
    return EXIT_FAILURE;
  }
  gettimeofday(&t1, nullptr);
  std::printf("PRACH configured in %llu us\n", us_since(t0, t1));

  // ---- Open RF device via source.h (device + device_args come from YAML) ----
  rf_source_t* rf = rf_open(args.rf.device.c_str(), args.rf.device_args.c_str());
  if (!rf) {
    std::fprintf(stderr, "rf_open(%s, %s) failed\n",
                 args.rf.device.c_str(), args.rf.device_args.c_str());
    srsran_prach_free(&prach);
    return EXIT_FAILURE;
  }

  // Zero the device timebase (no GPSDO/PPS in this simple example)
  if (rf_set_time_now(rf, 0.0) != 0) {
    std::fprintf(stderr, "rf_set_time_now() failed\n");
    //rf_close(rf);
    srsran_prach_free(&prach);
    return EXIT_FAILURE;
  }

  // ---- Generate, TX, and local-detect each preamble 0..63 ----
  uint32_t indices[64] = {0};
  uint32_t n_indices   = 0;

  for (uint32_t seq_index = 0; seq_index < 64; ++seq_index) {
    // Generate PRACH: [CP (N_cp) | sequence (N_seq)]
    if (srsran_prach_gen(&prach, seq_index, 0 /* freq-shift idx */, preamble) != SRSRAN_SUCCESS) {
      std::fprintf(stderr, "srsran_prach_gen failed at seq=%u\n", seq_index);
      //rf_close(rf);
      srsran_prach_free(&prach);
      return EXIT_FAILURE;
    }

    // Build burst descriptor
    PrachBurst burst;
    burst.samples        = preamble;
    burst.nsamps         = prach.N_cp + prach.N_seq;
    burst.tx_rate_hz     = args.g_tx_rate;
    burst.center_freq_hz = args.g_center_freq_hz;
    burst.tx_gain_db     = args.g_tx_gain_db;
    burst.start_in_s     = 0.050; // 50 ms in the future

    try {
      tx_send_prach(rf, burst);
    } catch (const std::exception& e) {
      std::fprintf(stderr, "TX error: %s\n", e.what());
      //rf_close(rf);
      srsran_prach_free(&prach);
      return EXIT_FAILURE;
    }

    // Local verify: detect (skip CP)
    gettimeofday(&t0, nullptr);
    n_indices = 0;
    if (srsran_prach_detect(&prach,
                            0,                        // time shift (samples)
                            &preamble[prach.N_cp],    // pointer to start of sequence
                            prach.N_seq,              // sequence length
                            indices, &n_indices) != SRSRAN_SUCCESS) {
      std::fprintf(stderr, "srsran_prach_detect failed at seq=%u\n", seq_index);
      //rf_close(rf);
      srsran_prach_free(&prach);
      return EXIT_FAILURE;
    }
    gettimeofday(&t1, nullptr);

    std::printf("seq=%2u  detect_time=%6llu us  found=%u",
                seq_index, us_since(t0, t1), n_indices);

    if (n_indices == 1 && indices[0] == seq_index) {
      std::printf("  [OK]\n");
    } else {
      std::printf("  [MISMATCH: expected %u]\n", seq_index);
      //rf_close(rf);
      srsran_prach_free(&prach);
      return EXIT_FAILURE;
    }
  }

  //rf_close(rf);
  srsran_prach_free(&prach);
  std::printf("All preambles 0..63 generated, transmitted, and verified. Done.\n");
  return EXIT_SUCCESS;
}
