/**
 * Copyright 2013-2023 Software Radio Systems Limited
 *
 * This file is part of srsRAN.
 *
 * srsRAN is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * srsRAN is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * A copy of the GNU Affero General Public License can be found in
 * the LICENSE file in the top-level directory of this distribution
 * and at http://www.gnu.org/licenses/.
 *
 */

#include <complex.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/metadata.hpp>
#include <vector>
#include <complex>

#include "srsran/srsran.h"

#define MAX_LEN 70176

static bool     is_nr            = false;
static uint32_t nof_prb          = 50;
static uint32_t config_idx       = 3;
static uint32_t root_seq_idx     = 0;
static uint32_t zero_corr_zone   = 15;
static uint32_t num_ra_preambles = 0; // use default

static void usage(char* prog)
{
  printf("Usage: %s\n", prog);
  printf("\t-n Uplink number of PRB [Default %d]\n", nof_prb);
  printf("\t-f Preamble format [Default 0]\n");
  printf("\t-r Root sequence index [Default 0]\n");
  printf("\t-z Zero correlation zone config [Default 1]\n");
  printf("\t-N Toggle LTE/NR operation, zero for LTE, non-zero for NR [Default %s]\n", is_nr ? "NR" : "LTE");
}

static void parse_args(int argc, char** argv)
{
  int opt;
  while ((opt = getopt(argc, argv, "nfrzN")) != -1) {
    switch (opt) {
      case 'n':
        nof_prb = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'f':
        config_idx = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'r':
        root_seq_idx = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'z':
        zero_corr_zone = (uint32_t)strtol(argv[optind], NULL, 10);
        break;
      case 'N':
        is_nr = (uint32_t)strtol(argv[optind], NULL, 10) > 0;
        break;
      default:
        usage(argv[0]);
        exit(-1);
    }
  }
}

// void transmission(uhd::usrp::multi_usrp::sptr usrp, const all_args_t args) {

//   // Configure the USRP transmission stream
//   uhd::stream_args_t stream_args("fc32",
//                                  "sc16"); // Complex float to short conversion
//   uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);

//   uhd::tx_metadata_t metadata;
//   metadata.start_of_burst =
//       true; // First packet should have start_of_burst = true
//   metadata.end_of_burst = false;
//   metadata.has_time_spec = false;

//   std::vector<std::complex<float>> samples = generateComplexSineWave(args);
  
//   while (true) {

//     // Transmit samples
//     tx_stream->send(samples.data(), samples.size(), metadata);
//     std::cout << "Transmitting...." << std::endl;

//     // After the first packet, set `start_of_burst = false`
//     metadata.start_of_burst = false;
//   }

//   // We will never reach this point unless we manually break the loop
// }

// Works with: transmission(rf_dev.usrp, args);
// Assumes you've already set rate/freq/gain before calling it (as in your main).


// forward-declared from your code:
std::vector<std::complex<float>> generateComplexSineWave(const all_args_t& args);

static void transmission(const uhd::usrp::multi_usrp::sptr& usrp,
                         const all_args_t& args)
{
  // 1) Build TX streamer (fc32->sc16 on the wire)
  uhd::stream_args_t sargs("fc32", "sc16");
  // If you want a specific channel, set sargs.channels = {channel_index};
  uhd::tx_streamer::sptr tx = usrp->get_tx_stream(sargs);
  const size_t mtu = tx->get_max_num_samps();

  // 2) Schedule a start time slightly in the future
  const auto now     = usrp->get_time_now();
  const auto tx_time = now + uhd::time_spec_t(0.05); // 50 ms guard

  uhd::tx_metadata_t md{};
  md.has_time_spec  = true;
  md.time_spec      = tx_time;
  md.start_of_burst = true;
  md.end_of_burst   = false;

  // 3) Get samples (your helper)
  std::vector<std::complex<float>> samples = generateComplexSineWave(args);

  // 4) Chunked send
  size_t offset = 0;
  while (offset < samples.size()) {
    const size_t to_send = std::min(mtu, samples.size() - offset);
    const void*  ptr     = static_cast<const void*>(&samples[offset]);

    const size_t sent = tx->send(ptr, to_send, md);
    if (sent != to_send) throw std::runtime_error("short send");

    md.start_of_burst = false;
    md.has_time_spec  = false;
    offset += sent;
  }

  // 5) End of burst
  md.end_of_burst = true;
  tx->send(nullptr, 0, md);
}

// using cfloat = std::complex<float>;

// // Send one PRACH burst (CP + sequence) at a scheduled time
// static void tx_send_prach(uhd::usrp::multi_usrp::sptr usrp,
//                           const cfloat* buf, size_t nsamps_total,
//                           double tx_rate, double center_freq_hz, double tx_gain_db,
//                           double seconds_in_future = 0.050 /* 50 ms guard */)
// {
//   // 1) Configure the USRP (rate/freq/gain). Do this once in real code, but safe here.
//   usrp->set_tx_rate(tx_rate);
//   usrp->set_tx_gain(tx_gain_db);
//   usrp->set_tx_freq(uhd::tune_request_t(center_freq_hz));

//   // 2) Create a TX streamer. Wire format sc16 is common; UHD will convert from fc32.
//   uhd::stream_args_t stream_args("fc32", "sc16");
//   uhd::tx_streamer::sptr tx_stream = usrp->get_tx_stream(stream_args);
//   const size_t mtu = tx_stream->get_max_num_samps();

//   // 3) Compute an absolute TX time (schedule a little in the future).
//   const uhd::time_spec_t now = usrp->get_time_now();
//   const uhd::time_spec_t tx_time = now + uhd::time_spec_t(seconds_in_future);

//   // 4) Prepare UHD metadata for a timestamped, finite burst.
//   uhd::tx_metadata_t md;
//   md.has_time_spec  = true;
//   md.time_spec      = tx_time;
//   md.start_of_burst = true;
//   md.end_of_burst   = false;

//   // 5) Chunk and send the burst (CP + sequence).
//   size_t offset = 0;
//   while (offset < nsamps_total) {
//     const size_t to_send = std::min(mtu, nsamps_total - offset);
//     const cfloat* chunk  = buf + offset;

//     // Send this chunk
//     const size_t sent = tx_stream->send(chunk, to_send, md);

//     if (sent != to_send) {
//       throw std::runtime_error("Short send on PRACH burst");
//     }

//     // After first packet, clear start_of_burst; keep time_spec only on first pkt.
//     md.start_of_burst = false;
//     md.has_time_spec  = false;

//     offset += sent;
//   }

//   // 6) Signal end of burst with a zero-length send (or mark end on last packet).
//   md.end_of_burst = true;
//   tx_stream->send("", 0, md);
// }



int main(int argc, char** argv)
{
  parse_args(argc, argv);
  srsran_prach_t prach;

  bool high_speed_flag = false;

  cf_t preamble[MAX_LEN];
  memset(preamble, 0, sizeof(cf_t) * MAX_LEN);

  ////builds/sets prach configurations
  srsran_prach_cfg_t prach_cfg;
  ZERO_OBJECT(prach_cfg);
  prach_cfg.is_nr            = is_nr;
  prach_cfg.config_idx       = config_idx;
  prach_cfg.hs_flag          = high_speed_flag;
  prach_cfg.freq_offset      = 0;
  prach_cfg.root_seq_idx     = root_seq_idx;
  prach_cfg.zero_corr_zone   = zero_corr_zone;
  prach_cfg.num_ra_preambles = num_ra_preambles;

  //initalize pratch object
  if (srsran_prach_init(&prach, srsran_symbol_sz(nof_prb))) {
    return -1;
  }

  //time measure/config
  struct timeval t[3] = {};
  gettimeofday(&t[1], NULL);
  //measures how long config takes?
  if (srsran_prach_set_cfg(&prach, &prach_cfg, nof_prb)) {
    ERROR("Error initiating PRACH object");
    return -1;
  }
  gettimeofday(&t[2], NULL);
  get_time_interval(t);
  printf("It took %ld microseconds to configure\n", t[0].tv_usec + t[0].tv_sec * 1000000UL);

  uint32_t seq_index = 0;
  uint32_t indices[64];
  uint32_t n_indices = 0;
  for (int i = 0; i < 64; i++)
    indices[i] = 0;

  for (seq_index = 0; seq_index < 64; seq_index++) {
    //generates prach signal for that index/preamble
    srsran_prach_gen(&prach, seq_index, 0, preamble);

    // ---------- INSERT TX PATH HERE ----------
    //   preamble points to complex baseband samples:
    //   [ CP (prach.N_cp samples) | sequence (prach.N_seq samples) ]
    // TX params — set these to your lab setup
    double tx_rate        = /* sample rate your device will use, e.g., 1.92e6 for LTE PRACH, or what matches srsran_symbol_sz(nof_prb) */;
    double center_freq_hz = /* your uplink center freq in Hz, lab/cabled */;
    double tx_gain_db     = /* conservative gain for cabled setup, e.g., -10 to +10 depending on attenuators */;

    // Send the full PRACH (CP + sequence)
    size_t nsamps_total = prach.N_cp + prach.N_seq;
    tx_send_prach(usrp, (const std::complex<float>*)preamble, nsamps_total,
                  tx_rate, center_freq_hz, tx_gain_db, 0.050 /* 50 ms in future */);
    uint32_t prach_len = prach.N_seq;

    gettimeofday(&t[1], NULL);
    //detect recieved signal
    srsran_prach_detect(&prach, 0, &preamble[prach.N_cp], prach_len, indices, &n_indices);
    gettimeofday(&t[2], NULL);
    get_time_interval(t);
    printf("texec=%ld us\n", t[0].tv_usec);
    if (n_indices != 1 || indices[0] != seq_index)
      return -1;
  }
  //free?

  srsran_prach_free(&prach);

  printf("Done\n");
  exit(0);
}
