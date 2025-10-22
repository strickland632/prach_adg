#include "args.h"
#include <string>

#define REQUIRE_FIELD(node, key)                                               \
  if (!node[#key]) {                                                           \
    throw std::runtime_error("Missing required field in config: '" #key "'");  \
  }
// args.cpp
bool     all_args_s::is_nr = false;
uint32_t all_args_s::nof_prb = 0;
uint32_t all_args_s::config_idx = 0;
uint32_t all_args_s::root_seq_idx = 0;
uint32_t all_args_s::zero_corr_zone = 0;
uint32_t  all_args_s::freq_offset = 0;
uint32_t all_args_s::num_ra_preambles = 0;
double   all_args_s::g_tx_rate = 0.0;
double   all_args_s::g_center_freq_hz = 0.0;
double   all_args_s::g_tx_gain_db = 0.0;

// all_args_t parseConfig(
//     const std::string &filename) { // change the filename to the real filename

//   YAML::Node config =
//       YAML::LoadFile(filename); // change the filename to its real filename

all_args_t parseConfig(
    const std::string &filename) { // change the filename to the real filename

  YAML::Node config =
      YAML::LoadFile(filename); // change the filename to its real filename

  REQUIRE_FIELD(config, is_nr);
  REQUIRE_FIELD(config, nof_prb);
  REQUIRE_FIELD(config, config_idx);
  REQUIRE_FIELD(config, root_seq_idx);
  REQUIRE_FIELD(config, zero_corr_zone);
  REQUIRE_FIELD(config, freq_offset);
  REQUIRE_FIELD(config, num_ra_preambles);

  REQUIRE_FIELD(config, g_tx_rate);
  REQUIRE_FIELD(config, g_center_freq_hz);
  REQUIRE_FIELD(config, g_tx_gain_db);


  REQUIRE_FIELD(config, rf);
  REQUIRE_FIELD(config, device);
  REQUIRE_FIELD(config, device_args);
  REQUIRE_FIELD(config, filepath);


  all_args_t args; // an instance of struct
  args.is_nr        = config["is_nr"].as<bool>();
  args.nof_prb      = config["nof_prb"].as<uint32_t>();
  args.config_idx  = config["config_idx"].as<uint32_t>();
  args.root_seq_idx = config["root_seq_idx"].as<uint32_t>();
  args.zero_corr_zone        = config["zero_corr_zone"].as<uint32_t>();
  args.freq_offset    = config["freq_offset"].as<uint32_t>();
  args.num_ra_preambles    = config["num_ra_preambles"].as<uint32_t>();

  args.g_tx_rate   = config["g_tx_rate"].as<double>();
  args.g_center_freq_hz  = config["og_center_freq_hz"].as<double>();
  args.g_tx_gain_db         = config["g_tx_gain_db"].as<double>();
  //rf_args_s rf_args;
  // rf_args.device = congif["rf"]["device"].as<uint32_t>();
  // rf_args.device_args = congif["rf"]["device_args"].as<uint32_t>();
  // rf_args.filepath = congif["rf"]["filepath"].as<std::string>();
  args.rf.device = config["rf"]["device"].as<uint32_t>();
  args.rf.device_args = config["rf"]["device_args"].as<uint32_t>();
  args.rf.filepath = config["rf"]["filepath"].as<std::string>();
  return args;
}

// void overrideConfig(all_args_t &args, int argc, char *argv[]) {
//   for (int i = 1; i < argc; ++i) {

//     if (std::strcmp(argv[i], "--amplitude") == 0 && i + 1 < argc) {
//       args.amplitude = std::atof(argv[++i]);
//     } else if (std::strcmp(argv[i], "--amplitude_width") == 0 &&
//                i + 1 < argc) {
//       args.amplitude_width = std::atof(argv[++i]);
//     } else if (std::strcmp(argv[i], "--center_frequency") == 0 &&
//                i + 1 < argc) {
//       args.center_frequency = std::atof(argv[++i]);
//     } else if (std::strcmp(argv[i], "--bandwidth") == 0 && i + 1 < argc) {
//       args.bandwidth = std::atof(argv[++i]);
//     } else if (std::strcmp(argv[i], "--num_samples") == 0 && i + 1 < argc) {
//       args.num_samples = std::atof(argv[++i]);
//     } else if (std::strcmp(argv[i], "--initial_phase") == 0 && i + 1 < argc) {
//       args.initial_phase = std::atof(argv[++i]);
//     } else if (std::strcmp(argv[i], "--sampling_freq") == 0 && i + 1 < argc) {
//       args.sampling_freq = std::atof(argv[++i]);
//     } else if (std::strcmp(argv[i], "--output_iq_file") == 0 && i + 1 < argc) {
//       args.output_iq_file = argv[++i];
//     } else if (std::strcmp(argv[i], "--output_csv_file") == 0 && i + 1 < argc) {
//       args.output_csv_file = argv[++i];
//     } else if (std::strcmp(argv[i], "--write_iq") == 0 && i + 1 < argc) {
//       args.write_iq = (std::string(argv[++i]) == "true");
//     } else if (std::strcmp(argv[i], "--write_csv") == 0 && i + 1 < argc) {
//       args.write_csv = (std::string(argv[++i]) == "true");
//     } else if (std::strcmp(argv[i], "--device_args") == 0 && i + 1 < argc) {
//       args.rf.device_args = std::string(argv[++i]);
//     } else if (std::strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
//       // Already processed separately, so skip here.
//       ++i;
//     } else if (std::strcmp(argv[i], "--tx_gain") == 0 && i + 1 < argc) {
//       args.rf.tx_gain = std::stof(argv[++i]);
//     } else {
//       std::cerr << "Unknown or incomplete option: " << argv[i] << std::endl;
//     }
//   }
// }