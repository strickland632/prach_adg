#ifndef ARGS_H
#define ARGS_H

#include <cstring> 
#include <iostream>
#include <yaml-cpp/yaml.h>



typedef struct rf_args_s {
  std::string device_args;
  float tx_gain;
  std::string filepath;
  std::string device;

} rf_args_t;
//different for zmg/uhd? or do that in main?
typedef struct all_args_s {
    //prach config details - add one for number of 
    static bool     is_nr;
    static uint32_t nof_prb; //physical resource block - i dunno
    static uint32_t config_idx; //0-255?
    static uint32_t root_seq_idx; //from/in SIB 0-837
    static uint32_t zero_corr_zone; //determined in RRC message 0-15
    static uint32_t freq_offset; //0 default
    static uint32_t num_ra_preambles; // 0 is default
    //these are rf details?
    static double   g_tx_rate; //        = 1.92e6;   
    static double   g_center_freq_hz; // = 1.850e9;  // uplink center freq
    static double   g_tx_gain_db; //     = 0.0;      // cables/attenuators
    rf_args_t rf;

//   float amplitude;
//   float amplitude_width;
//   float center_frequency;
//   float bandwidth;
//   float initial_phase;
//   size_t num_samples;
//   float sampling_freq;
//   std::string output_iq_file;
//   std::string output_csv_file;
//   bool write_iq;
//   bool write_csv;
//   rf_args_t rf;
} all_args_t;


all_args_t parseConfig(const std::string &filename);

// void overrideConfig(all_args_t &args, int argc, char *argv[]);

#endif // !ARGS_H