// with .iq


#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <complex>
#include <string>
#include <vector>
#include <fstream>
#include <cmath>
#include <sys/time.h>


extern "C" {
#include "srsran/srsran.h"
}


#include "args.h"
#include "source.h"       // Source, create_source_instance, cf_t_1
#include "uhd_source.h"   // for UHDSource declaration (to define send here)


#define MAX_LEN 70176


// tiny time helper
static inline unsigned long long us_since(const timeval& a, const timeval& b)
{
 return (unsigned long long)(b.tv_sec - a.tv_sec) * 1000000ULL +
        (unsigned long long)(b.tv_usec - a.tv_usec);
}


// dump CF32LE (std::complex<float>) to file
static bool dump_cf32le(const std::string& path, const std::complex<float>* data, size_t nsamps)
{
  std::ofstream f(path, std::ios::binary);
  if (!f) return false;
  f.write(reinterpret_cast<const char*>(data), nsamps * sizeof(std::complex<float>));
  return static_cast<bool>(f);
}


// dump SC16 interleaved .iq (I,Q as int16 little-endian)
static bool dump_sc16_iq(const std::string& path, const std::complex<float>* data, size_t nsamps)
{
  fprintf(stderr, "Trung to dump to file%f", data);
 std::ofstream f(path, std::ios::binary);
 if (!f) return false;


 const float scale = 0.8f * 32767.0f;
 for (size_t i = 0; i < nsamps; ++i) {
   float I = std::max(-1.0f, std::min(1.0f, data[i].real()));
   float Q = std::max(-1.0f, std::min(1.0f, data[i].imag()));
   int16_t i16 = static_cast<int16_t>(std::lrintf(I * scale));
   int16_t q16 = static_cast<int16_t>(std::lrintf(Q * scale));
   f.write(reinterpret_cast<const char*>(&i16), sizeof(i16));
   f.write(reinterpret_cast<const char*>(&q16), sizeof(q16));
 }
 fprintf(stderr, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
 return static_cast<bool>(f);
}


// provide validate<T> template body here (+ explicit instantiations) so uses from other TUs can link
template<typename T>
T validate(const YAML::Node node, const std::string key){
 if (!node[key]) {
   throw std::runtime_error("Missing required key: '" + key + "'");
 }
 try {
   return node[key].as<T>();
 } catch (const YAML::TypedBadConversion<T>&) {
   throw std::runtime_error("Key '" + key + "' is not of the expected type.");
 }
}


// concrete instantiations needed by other translation units
template std::string validate<std::string>(YAML::Node, std::string);
template double      validate<double>(YAML::Node, std::string);


// define UHDSource::send(...) here to satisfy vtable if it's not defined elsewhere
source_error_t UHDSource::send(cf_t_1* /*buffer*/, size_t /*nof_samples*/)
{
 source_error_t ok; ok.type = SOURCE_SUCCESS; ok.msg.clear();
 return ok;
}


int main(int argc, char** argv)
{
 // parse CLI: --config or -c
 std::string config_file = "basic_prach.yaml";
 for (int i = 1; i < argc; ++i) {
   if ((std::strcmp(argv[i], "--config") == 0 || std::strcmp(argv[i], "-c") == 0) && i + 1 < argc) {
     config_file = argv[++i];
     break;
   }
 }
 if (config_file.empty()) {
   std::fprintf(stderr, "Usage: %s --config <basic_prach.yaml>\n", argv[0]);
   return EXIT_FAILURE;
 }


 // load app config
 all_args_t args{};
 try {
   args = parseConfig(config_file);
 } catch (const std::exception& e) {
   std::fprintf(stderr, "Config error: %s\n", e.what());
   return EXIT_FAILURE;
 }


 // build rf_config for UHDSource::create()
 YAML::Node rf_cfg;
 rf_cfg["rf_args"] = args.rf.device_args;
 rf_cfg["srate"]   = args.g_tx_rate;
 rf_cfg["freq"]    = args.g_center_freq_hz;
 rf_cfg["gain"]    = args.g_tx_gain_db;


 // create device via factory
 std::unique_ptr<Source> src;
 try {
   src = create_source_instance(args.rf.device);
 } catch (const std::exception& e) {
   std::fprintf(stderr, "create_source_instance failed: %s\n", e.what());
   return EXIT_FAILURE;
 }
 if (!src) {
   std::fprintf(stderr, "create_source_instance returned null\n");
   return EXIT_FAILURE;
 }


 if (auto err = src->create(rf_cfg); err.type != SOURCE_SUCCESS) {
   std::fprintf(stderr, "Source::create failed: %s\n", err.msg.c_str());
   return EXIT_FAILURE;
 }


 // PRACH cfg (srsRAN)
 srsran_prach_t     prach = {};
 srsran_prach_cfg_t prach_cfg;
 ZERO_OBJECT(prach_cfg);


 prach_cfg.is_nr            = args.is_nr;
 prach_cfg.config_idx       = args.config_idx;
 prach_cfg.hs_flag          = false;
 prach_cfg.freq_offset      = args.freq_offset;
 prach_cfg.root_seq_idx     = args.root_seq_idx;
 prach_cfg.zero_corr_zone   = args.zero_corr_zone;
 prach_cfg.num_ra_preambles = args.num_ra_preambles;


 // PRACH working buffer (srsRAN uses cf_t = float complex)
 cf_t preamble[MAX_LEN];
 std::memset(preamble, 0, sizeof(preamble));


 // init srsRAN PRACH
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


 // RX capture buffer (std::complex<float>)
 const size_t CAPTURE_SAMPS = 200000;
 std::vector<std::complex<float>> rx(CAPTURE_SAMPS);


 // blocking receive (duration depends on Source::recv implementation)
 std::printf("Starting RX capture of %zu samples into rx_capture.iq ...\n", CAPTURE_SAMPS);
 if (auto err = src->recv(rx.data(), rx.size()); err.type != SOURCE_SUCCESS) {
   std::fprintf(stderr, "Source::recv failed: %s (continuing)\n", err.msg.c_str());
 }


 // generate, TX, and local-detect each preamble 0..63
 uint32_t indices[64] = {0};
 uint32_t n_indices   = 0;


 for (uint32_t seq_index = 0; seq_index < 64; ++seq_index) {
   if (srsran_prach_gen(&prach, seq_index, 0, preamble) != SRSRAN_SUCCESS) {
     std::fprintf(stderr, "srsran_prach_gen failed at seq=%u\n", seq_index);
     srsran_prach_free(&prach);
     return EXIT_FAILURE;
   }


   const uint32_t nsamps = prach.N_cp + prach.N_seq;
   if (nsamps > MAX_LEN) {
     std::fprintf(stderr, "PRACH buffer overflow (N_cp+N_seq=%u > MAX_LEN=%u)\n",
                  nsamps, (unsigned)MAX_LEN);
     srsran_prach_free(&prach);
     return EXIT_FAILURE;
   }


   // convert PRACH buffer (cf_t) -> std::complex<float> for Source::send
   std::vector<std::complex<float>> tx(nsamps);
   std::memcpy(tx.data(), preamble, nsamps * sizeof(std::complex<float>));


   // dump first TX burst to .iq for reference
   if (seq_index == 0) {
     if (dump_sc16_iq("./out/tx_prach.iq", tx.data(), tx.size())) {
       std::printf("Wrote %zu samples to tx_prach.iq\n", tx.size());
     }
     if (dump_cf32le("./out/tx_prach.cf32", tx.data(), tx.size())){
       std::printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!Wrote %zu samples to tx_prach.cf32\n", tx.size());
     }
   }


   // immediate transmit
   if (auto err = src->send(tx.data(), tx.size()); err.type != SOURCE_SUCCESS) {
     std::fprintf(stderr, "Source::send failed at seq=%u: %s\n", seq_index, err.msg.c_str());
     srsran_prach_free(&prach);
     return EXIT_FAILURE;
   }


//   local verify: detect (skip CP)
   gettimeofday(&t0, nullptr);
   n_indices = 0;
   if (srsran_prach_detect(&prach,
                           0,
                           &preamble[prach.N_cp],
                           prach.N_seq,
                           indices, &n_indices) != SRSRAN_SUCCESS) {
     std::fprintf(stderr, "srsran_prach_detect failed at seq=%u\n", seq_index);
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
     srsran_prach_free(&prach);
     return EXIT_FAILURE;
   }
 }


//  write RX capture to disk as SC16 .iq and also keep a float32 copy if desired
  if (!rx.empty()) {
    if (dump_sc16_iq("./out/rx_capture.iq", rx.data(), rx.size())) {
      std::printf("Wrote %zu samples to rx_capture.iq\n", rx.size());
    } else {
      std::fprintf(stderr, "Failed to write RX capture to rx_capture.iq\n");
    }
    if (dump_cf32le("./out/rx_capture.cf32", rx.data(), rx.size())){
       std::printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!Wrote %zu samples to rx_capture.cf32 \n", rx.size());
     }
  //  uncomment next lines if you also want float32 version for analysis
  //  if (dump_cf32le("rx_capture.cf32le", rx.data(), rx.size())) {
  //    std::printf("Wrote %zu samples to rx_capture.cf32le\n", rx.size());
  //  }
  }


 srsran_prach_free(&prach);
 std::printf("All preambles 0..63 generated, transmitted, verified, and RX/TX IQ files saved. Done.\n");
 return EXIT_SUCCESS;
}








// #include <cstdio>
// #include <cstdlib>
// #include <cstdint>
// #include <cstring>
// #include <stdexcept>
// #include <algorithm>
// #include <complex>
// #include <string>
// #include <vector>
// #include <fstream>
// #include <sys/time.h>

// extern "C" {
// #include "srsran/srsran.h"
// }

// #include "args.h"
// #include "source.h"       // Source, create_source_instance, cf_t_1
// #include "uhd_source.h"   // for UHDSource declaration (to define send here)

// #define MAX_LEN 70176

// // tiny time helper
// static inline unsigned long long us_since(const timeval& a, const timeval& b)
// {
//   return (unsigned long long)(b.tv_sec - a.tv_sec) * 1000000ULL +
//          (unsigned long long)(b.tv_usec - a.tv_usec);
// }

// // dump CF32LE (std::complex<float>) to file
// static bool dump_cf32le(const std::string& path, const std::complex<float>* data, size_t nsamps)
// {
//   std::ofstream f(path, std::ios::binary);
//   if (!f) return false;
//   f.write(reinterpret_cast<const char*>(data), nsamps * sizeof(std::complex<float>));
//   return static_cast<bool>(f);
// }

// // provide validate<T> template body here (+ explicit instantiations) so uses from other TUs can link
// template<typename T>
// T validate(const YAML::Node node, const std::string key){
//   if (!node[key]) {
//     throw std::runtime_error("Missing required key: '" + key + "'");
//   }
//   try {
//     return node[key].as<T>();
//   } catch (const YAML::TypedBadConversion<T>&) {
//     throw std::runtime_error("Key '" + key + "' is not of the expected type.");
//   }
// }

// // concrete instantiations needed by other translation units
// template std::string validate<std::string>(YAML::Node, std::string);
// template double      validate<double>(YAML::Node, std::string);

// // define UHDSource::send(...) here to satisfy vtable if it's not defined elsewhere
// source_error_t UHDSource::send(cf_t_1* /*buffer*/, size_t /*nof_samples*/)
// {
//   source_error_t ok; ok.type = SOURCE_SUCCESS; ok.msg.clear();
//   return ok;
// }

// int main(int argc, char** argv)
// {
//   // parse CLI: --config or -c
//   std::string config_file = "basic_prach.yaml";
//   for (int i = 1; i < argc; ++i) {
//     if ((std::strcmp(argv[i], "--config") == 0 || std::strcmp(argv[i], "-c") == 0) && i + 1 < argc) {
//       config_file = argv[++i];
//       break;
//     }
//   }
//   if (config_file.empty()) {
//     std::fprintf(stderr, "Usage: %s --config <basic_prach.yaml>\n", argv[0]);
//     return EXIT_FAILURE;
//   }

//   // load app config
//   all_args_t args{};
//   try {
//     args = parseConfig(config_file);
//   } catch (const std::exception& e) {
//     std::fprintf(stderr, "Config error: %s\n", e.what());
//     return EXIT_FAILURE;
//   }

//   // build rf_config for UHDSource::create()
//   // expects keys: "rf_args" (string), "srate" (double), "freq" (double), "gain" (double)
//   YAML::Node rf_cfg;
//   rf_cfg["rf_args"] = args.rf.device_args;
//   rf_cfg["srate"]   = args.g_tx_rate;
//   rf_cfg["freq"]    = args.g_center_freq_hz;
//   rf_cfg["gain"]    = args.g_tx_gain_db;

//   // create device via factory
//   std::unique_ptr<Source> src;
//   try {
//     src = create_source_instance(args.rf.device);
//   } catch (const std::exception& e) {
//     std::fprintf(stderr, "create_source_instance failed: %s\n", e.what());
//     return EXIT_FAILURE;
//   }
//   if (!src) {
//     std::fprintf(stderr, "create_source_instance returned null\n");
//     return EXIT_FAILURE;
//   }

//   if (auto err = src->create(rf_cfg); err.type != SOURCE_SUCCESS) {
//     std::fprintf(stderr, "Source::create failed: %s\n", err.msg.c_str());
//     return EXIT_FAILURE;
//   }

//   // PRACH cfg (srsRAN)
//   srsran_prach_t     prach = {};
//   srsran_prach_cfg_t prach_cfg;
//   ZERO_OBJECT(prach_cfg);

//   prach_cfg.is_nr            = args.is_nr;
//   prach_cfg.config_idx       = args.config_idx;
//   prach_cfg.hs_flag          = false;
//   prach_cfg.freq_offset      = args.freq_offset;
//   prach_cfg.root_seq_idx     = args.root_seq_idx;
//   prach_cfg.zero_corr_zone   = args.zero_corr_zone;
//   prach_cfg.num_ra_preambles = args.num_ra_preambles;

//   // PRACH working buffer (srsRAN uses cf_t = float complex)
//   cf_t preamble[MAX_LEN];
//   std::memset(preamble, 0, sizeof(preamble));

//   // init srsRAN PRACH
//   if (srsran_prach_init(&prach, srsran_symbol_sz(args.nof_prb)) != SRSRAN_SUCCESS) {
//     std::fprintf(stderr, "Failed to init PRACH object\n");
//     return EXIT_FAILURE;
//   }

//   timeval t0{}, t1{};
//   gettimeofday(&t0, nullptr);
//   if (srsran_prach_set_cfg(&prach, &prach_cfg, args.nof_prb) != SRSRAN_SUCCESS) {
//     std::fprintf(stderr, "Error configuring PRACH\n");
//     srsran_prach_free(&prach);
//     return EXIT_FAILURE;
//   }
//   gettimeofday(&t1, nullptr);
//   std::printf("PRACH configured in %llu us\n", us_since(t0, t1));

//   // RX capture buffer (std::complex<float>)
//   const size_t CAPTURE_SAMPS = 200000;
//   std::vector<std::complex<float>> rx(CAPTURE_SAMPS);

//   // blocking receive (duration depends on Source::recv implementation)
//   std::printf("Starting RX capture of %zu samples into rx_capture.cf32le ...\n", CAPTURE_SAMPS);
//   if (auto err = src->recv(rx.data(), rx.size()); err.type != SOURCE_SUCCESS) {
//     std::fprintf(stderr, "Source::recv failed: %s (continuing)\n", err.msg.c_str());
//   }

//   // generate, TX, and local-detect each preamble 0..63
//   uint32_t indices[64] = {0};
//   uint32_t n_indices   = 0;

//   for (uint32_t seq_index = 0; seq_index < 64; ++seq_index) {
//     if (srsran_prach_gen(&prach, seq_index, 0, preamble) != SRSRAN_SUCCESS) {
//       std::fprintf(stderr, "srsran_prach_gen failed at seq=%u\n", seq_index);
//       srsran_prach_free(&prach);
//       return EXIT_FAILURE;
//     }

//     const uint32_t nsamps = prach.N_cp + prach.N_seq;
//     if (nsamps > MAX_LEN) {
//       std::fprintf(stderr, "PRACH buffer overflow (N_cp+N_seq=%u > MAX_LEN=%u)\n",
//                    nsamps, (unsigned)MAX_LEN);
//       srsran_prach_free(&prach);
//       return EXIT_FAILURE;
//     }

//     // convert PRACH buffer (cf_t) -> std::complex<float> for Source::send
//     std::vector<std::complex<float>> tx(nsamps);
//     std::memcpy(tx.data(), preamble, nsamps * sizeof(std::complex<float>));

//     // immediate transmit
//     if (auto err = src->send(tx.data(), tx.size()); err.type != SOURCE_SUCCESS) {
//       std::fprintf(stderr, "Source::send failed at seq=%u: %s\n", seq_index, err.msg.c_str());
//       srsran_prach_free(&prach);
//       return EXIT_FAILURE;
//     }

//     // // local verify: detect (skip CP)
//     // gettimeofday(&t0, nullptr);
//     // n_indices = 0;
//     // std::fprintf(stderr, "TESTING: seq=%u\n", seq_index);
//     // for (int element : indices) { // 'element' will take on the value of each array element
//     //     std::fprintf(stderr, "element: %u\n", element);
//     // }

//     // std::printf("  stats seq=%u ind=%u, n_in=%u\n", seq_index, indices[0], n_indices, n_indices);
//     // if (srsran_prach_detect(&prach,
//     //                         0,
//     //                         &preamble[prach.N_cp],
//     //                         prach.N_seq,
//     //                         indices, &n_indices) != SRSRAN_SUCCESS) {
//     //   std::fprintf(stderr, "srsran_prach_detect failed at seq=%u\n", seq_index);
//     //   srsran_prach_free(&prach);
//     //   return EXIT_FAILURE;
//     // }
//     // int result = srsran_prach_detect(&prach,
//     //                         0,
//     //                         &preamble[prach.N_cp],
//     //                         prach.N_seq,
//     //                         indices, &n_indices);
//     // std::printf("  result%i\n", result);
//     // std::printf("  stats seq=%u ind=%u, n_in=%u\n", seq_index, indices[0], n_indices, n_indices);
//     // gettimeofday(&t1, nullptr);
//     // std::printf("  stats seq=%u ind=%u, n_in=%u\n", seq_index, indices[0], n_indices, n_indices);
//     // std::printf("seq=%2u  detect_time=%6llu us  found=%u",
//     //             seq_index, us_since(t0, t1), n_indices);
//     // std::printf("  stats seq=%u ind=%u, n_in=%u\n", seq_index, indices[0], n_indices, n_indices);
//     // if (n_indices == 1 && indices[0] == seq_index) {
//     //   std::printf("  [OK]\n");
//     //   std::printf("  stats seq=%u ind=%u, n_in=%u\n", seq_index, indices[0], n_indices, n_indices);
//     // } else {
//     //   std::printf("  [MISMATCH: expected %u]\n", seq_index);
//     //   srsran_prach_free(&prach);
//     //   std::printf("  stats seq=%u ind=%u, n_in=%u\n", seq_index, indices[0], n_indices, n_indices);
//     //   return EXIT_FAILURE;
//     // }
//   }

//   // write the RX capture to disk
//   if (!rx.empty()) {
//     const std::string out_path = "/home/oaic/prach-agent/rx_capture.cf32le";
//     if (dump_cf32le(out_path, rx.data(), rx.size())) {
//       std::printf("Wrote %zu samples to %s\n", rx.size(), out_path.c_str());
//     } else {
//       std::fprintf(stderr, "Failed to write RX capture to %s\n", out_path.c_str());
//     }
//   }

//   srsran_prach_free(&prach);
//   std::printf("All preambles 0..63 generated, transmitted, verified, and RX capture saved. Done.\n");
//   return EXIT_SUCCESS;
// }


