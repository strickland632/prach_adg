

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <complex>
#include <sys/time.h>
#include <unistd.h>

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/metadata.hpp>

#include "args.h"
extern "C" {
#include "srsran/srsran.h"
}

#define MAX_LEN 70176

// ------- CLI defaults ------- //get these from the .yaml instead?
static bool     is_nr            = false;
static uint32_t nof_prb          = 50;
static uint32_t config_idx       = 3; //get directly
static uint32_t root_seq_idx     = 0;
static uint32_t zero_corr_zone   = 15; //get directly
static uint32_t num_ra_preambles = 0; //

// Example RF defaults — EDIT //get from config? sarah liz
static double   g_tx_rate        = 1.92e6;   
static double   g_center_freq_hz = 1.850e9;  // uplink center freq
static double   g_tx_gain_db     = 0.0;      // cables/attenuators

// --------- helpers ----------
static inline unsigned long long us_since(const timeval& a, const timeval& b) 
{
  return (unsigned long long)(b.tv_sec - a.tv_sec) * 1000000ULL +
         (unsigned long long)(b.tv_usec - a.tv_usec);
}

static void usage(const char* prog) 
{
  printf("Usage: %s [options]\n", prog);
  printf("  -n <PRB>        Uplink number of PRB (default %u)\n", nof_prb);
  printf("  -f <cfg_idx>    PRACH config index / format (default %u)\n", config_idx);
  printf("  -r <root_idx>   Root sequence index (default %u)\n", root_seq_idx);
  printf("  -z <zc_zone>    Zero correlation zone config (default %u)\n", zero_corr_zone);
  printf("  -N <0|1>        0=LTE, 1=NR (default %s)\n", is_nr ? "NR" : "LTE");
  printf("  --rate <Hz>     TX sample rate (default %.3f)\n", g_tx_rate);
  printf("  --freq <Hz>     TX center frequency (default %.3f)\n", g_center_freq_hz);
  printf("  --gain <dB>     TX gain (default %.1f)\n", g_tx_gain_db);
  printf("  --addr <str>    UHD device args (e.g., addr=192.168.10.2)\n");
}

//dont need to parse args from cmdjust pull from yalm

// static void parse_args(int argc, char** argv, std::string& dev_args) 
// {
//   // handle long options manually
//   for (int i = 1; i < argc; ++i) {
//     if (strcmp(argv[i], "--rate") == 0 && i + 1 < argc) {
//       g_tx_rate = atof(argv[++i]);
//     } else if (strcmp(argv[i], "--freq") == 0 && i + 1 < argc) {
//       g_center_freq_hz = atof(argv[++i]);
//     } else if (strcmp(argv[i], "--gain") == 0 && i + 1 < argc) {
//       g_tx_gain_db = atof(argv[++i]);
//     } else if (strcmp(argv[i], "--addr") == 0 && i + 1 < argc) {
//       dev_args = argv[++i];
//     }
//   }

//   // short opts for PRACH/srsRAN params
//   optind = 1; // reset for getopt
//   int opt;
//   while ((opt = getopt(argc, argv, "n:f:r:z:N:")) != -1) {
//     switch (opt) {
//       case 'n': nof_prb        = (uint32_t)strtoul(optarg, nullptr, 10); break;
//       case 'f': config_idx     = (uint32_t)strtoul(optarg, nullptr, 10); break;
//       case 'r': root_seq_idx   = (uint32_t)strtoul(optarg, nullptr, 10); break;
//       case 'z': zero_corr_zone = (uint32_t)strtoul(optarg, nullptr, 10); break;
//       case 'N': is_nr          = (uint32_t)strtoul(optarg, nullptr, 10) > 0; break;
//       default:  usage(argv[0]); exit(EXIT_FAILURE);
//     }
//   }
// }
//end read in
//UHD TX: send PRACH CP+sequence
//wrap inside of a struct before sending SARAH ELIZABETH
static void tx_send_prach(const uhd::usrp::multi_usrp::sptr& usrp,
                          const cf_t* buf, size_t nsamps_total,
                          double tx_rate, double center_freq_hz, double tx_gain_db,
                          double seconds_in_future = 0.050 /* 50 ms */)
{
  // Configure USRP (rate/freq/gain)
  usrp->set_tx_rate(tx_rate);
  usrp->set_tx_gain(tx_gain_db);
  usrp->set_tx_freq(uhd::tune_request_t(center_freq_hz));

  // TX streamer: fc32 (host) -> sc16 (wire)
  uhd::stream_args_t sargs("fc32", "sc16");
  auto tx = usrp->get_tx_stream(sargs);
  const size_t mtu = tx->get_max_num_samps();

  // Schedule start a bit in the future for timing
  const auto now     = usrp->get_time_now();
  const auto tx_time = now + uhd::time_spec_t(seconds_in_future);

  uhd::tx_metadata_t md{};
  md.has_time_spec  = true;
  md.time_spec      = tx_time;
  md.start_of_burst = true;
  md.end_of_burst   = false;

  // Chunked send
  size_t offset = 0;
  while (offset < nsamps_total) {
    const size_t to_send = std::min(mtu, nsamps_total - offset);
    const void*  chunk   = static_cast<const void*>(buf + offset);
    const void* buffs[] = { chunk };
    const size_t sent    = tx->send(buffs, to_send, md);

    if (sent != to_send) throw std::runtime_error("Short send on PRACH burst");

    md.start_of_burst = false;  // only first packet has SoB + time
    md.has_time_spec  = false;
    offset += sent;
  }

  // End of burst
  md.end_of_burst = true;
  const void* eob_buffs[] = { nullptr };
  tx->send(eob_buffs, 0, md);
}

int main(int argc, char** argv)
{
  std::string config_file = "basic_prach.yaml"; //path?

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
      config_file = argv[++i];
      break;
    }
  }

  if (config_file.empty()) {
    fprintf(stderr, "Usage: prach --config [config file]\n");
    return EXIT_FAILURE;
  }

  //raes decoded is what other guy is doingr messag
  // std::string dev_args; // e.g., "addr=192.168.10.2"
  // parse_args(argc, argv, dev_args);
  

  // Load config from YAML
  all_args_t args = parseConfig(config_file);

// //sarah delete this
//   // ---- Create USRP ----
//   auto usrp = uhd::usrp::multi_usrp::make(dev_args);
//   // Simple time base init (no GPSDO/PPS here)
//   usrp->set_time_now(uhd::time_spec_t(0.0));

  // ---- Build PRACH config ----
  srsran_prach_t     prach;
  srsran_prach_cfg_t prach_cfg;
  ZERO_OBJECT(prach_cfg);
//this stuff would be from args double check sarah liz
  prach_cfg.is_nr            = args.is_nr;
  prach_cfg.config_idx       = args.config_idx;
  prach_cfg.hs_flag          = false;     // high-speed flag
  prach_cfg.freq_offset      = 0;
  prach_cfg.root_seq_idx     = args.root_seq_idx;
  prach_cfg.zero_corr_zone   = args.zero_corr_zone;
  prach_cfg.num_ra_preambles = args.num_ra_preambles;

  // PRACH buffer (cf_t is srsRAN complex float)
  cf_t preamble[MAX_LEN];
  memset(preamble, 0, sizeof(preamble));

  // Init PRACH
  if (srsran_prach_init(&prach, srsran_symbol_sz(nof_prb)) != SRSRAN_SUCCESS) {
    fprintf(stderr, "Failed to init PRACH object\n");
    return EXIT_FAILURE;
  }

  // Configure PRACH
  timeval t0{}, t1{};
  gettimeofday(&t0, nullptr);
  if (srsran_prach_set_cfg(&prach, &prach_cfg, nof_prb) != SRSRAN_SUCCESS) {
    fprintf(stderr, "Error configuring PRACH object\n");
    srsran_prach_free(&prach);
    return EXIT_FAILURE;
  }
  gettimeofday(&t1, nullptr);
  printf("PRACH configured in %llu us\n", us_since(t0, t1));

  // Detection buffers
  uint32_t indices[64] = {0};
  uint32_t n_indices   = 0;

  while (true)
  {
    // ---- Generate, TX, and detect each preamble index ----
    for (uint32_t seq_index = 0; seq_index < 64; ++seq_index) {
      // Generate: preamble = [ CP (N_cp) | sequence (N_seq) ]
      if (srsran_prach_gen(&prach, seq_index, 0 /* freq-shift idx */, preamble) != SRSRAN_SUCCESS) {
        fprintf(stderr, "srsran_prach_gen failed at seq=%u\n", seq_index);
        srsran_prach_free(&prach);
        return EXIT_FAILURE;
      }
  //double check sarah liz
      // --- Transmit CP + sequence over UHD ---
      const size_t nsamps_total = prach.N_cp + prach.N_seq;
      tx_send_prach(usrp, preamble, nsamps_total,
                    args.g_tx_rate, args.g_center_freq_hz, args.g_tx_gain_db, 0.050 /* 50ms */);

      // --- Local detect (skip CP) for verification ---
      gettimeofday(&t0, nullptr);
      n_indices = 0;
      if (srsran_prach_detect(&prach,
                              0,                         // time shift (samples)
                              &preamble[prach.N_cp],     // pointer to start of sequence
                              prach.N_seq,               // sequence length
                              indices, &n_indices) != SRSRAN_SUCCESS) {
        fprintf(stderr, "srsran_prach_detect failed at seq=%u\n", seq_index);
        srsran_prach_free(&prach);
        return EXIT_FAILURE;
      }
      gettimeofday(&t1, nullptr);

      printf("seq=%2u  detect_time=%6llu us  found=%u",
            seq_index, us_since(t0, t1), n_indices);

      if (n_indices == 1 && indices[0] == seq_index) {
        printf("  [OK]\n");
      } else {
        printf("  [MISMATCH: expected %u]\n", seq_index);
        srsran_prach_free(&prach);
        return EXIT_FAILURE;
      }
    }
  }

  srsran_prach_free(&prach);
  printf("All preambles 0..63 generated, transmitted, and verified. Done.\n");
  return EXIT_SUCCESS;
}
