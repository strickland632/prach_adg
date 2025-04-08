#include "config.h"
#include "filesource.h"
#include "srsran/phy/sync/ssb.h"
#include <complex.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <srsran/phy/common/phy_common.h>
#include <srsran/phy/common/phy_common_nr.h>
#include <srsran/phy/phch/pbch_msg_nr.h>
#include <srsran/phy/utils/vector.h>
#include <stdlib.h>
#include <sys/types.h>

#define SSB_NOF_SAMPLES 10000

static srsran_ssb_pattern_t ssb_pattern = SRSRAN_SSB_PATTERN_A;
static srsran_subcarrier_spacing_t ssb_scs = srsran_subcarrier_spacing_15kHz;
static srsran_duplex_mode_t duplex_mode = SRSRAN_DUPLEX_MODE_FDD;

int main(int argc, char **argv) {
  // generic reader class
  // Open rf dev
  // search for ssb
  // decode pbch unpack mib
  // print mib
  cf_t *buffer = srsran_vec_cf_malloc(SSB_NOF_SAMPLES);

  if (argc != 2) {
    fprintf(stderr, "Usage: pdcch-agent <config file>\n");
    return SRSRAN_ERROR;
  }

  std::string config_path(argv[1]);
  agent_config_t conf = load(config_path);

  srsran_filesource_t fsrc = {};
  if (srsran_filesource_init(&fsrc, conf.rf.file_path.c_str(),
                             SRSRAN_COMPLEX_FLOAT_BIN) < SRSRAN_SUCCESS) {
    printf("Error opening file\n");
    return SRSRAN_ERROR;
  }

  // Initialise SSB
  srsran_ssb_t ssb = {};
  srsran_ssb_args_t ssb_args = {};
  ssb_args.enable_decode = true;
  ssb_args.enable_search = true;
  if (srsran_ssb_init(&ssb, &ssb_args) < SRSRAN_SUCCESS) {
    fprintf(stderr, "Init\n");
    return SRSRAN_ERROR;
  }

  srsran_ssb_cfg_t ssb_cfg = {};
  ssb_cfg.srate_hz = conf.rf.sample_rate;
  ssb_cfg.center_freq_hz = conf.rf.frequency;
  ssb_cfg.ssb_freq_hz = conf.rf.frequency;
  ssb_cfg.scs = ssb_scs;
  ssb_cfg.pattern = ssb_pattern;
  ssb_cfg.duplex_mode = duplex_mode;
  if (srsran_ssb_set_cfg(&ssb, &ssb_cfg) < SRSRAN_SUCCESS) {
    fprintf(stderr, "Error setting SSB configuration\n");
    return SRSRAN_ERROR;
  }

  while (srsran_filesource_read(&fsrc, buffer, SSB_NOF_SAMPLES) > 0) {
    uint32_t N_id = 0;
    srsran_csi_trs_measurements_t meas = {};
    if (srsran_ssb_csi_search(&ssb, buffer, SSB_NOF_SAMPLES, &N_id, &meas) <
        SRSRAN_SUCCESS) {
      printf("Error performing SSB-CSI search\n");
    }

    // Print measurement
    char str[512] = {};
    srsran_csi_meas_info(&meas, str, sizeof(str));
    if (meas.rsrp != 0)
      printf("CSI MEAS - search pci=%d %s\n", N_id, str);

    // Perform SSB search
    srsran_ssb_search_res_t search_res = {};
    if (srsran_ssb_search(&ssb, buffer, SSB_NOF_SAMPLES, &search_res) <
        SRSRAN_SUCCESS) {
      printf("Error performing SSB search\n");
    }

    // Print decoded PBCH message
    srsran_pbch_msg_info(&search_res.pbch_msg, str, sizeof(str));
    if (!search_res.pbch_msg.crc)
      continue;
    printf("SSB PBCH - t_offset=%d pci=%d %s crc=%s\n", search_res.t_offset,
           search_res.N_id, str, search_res.pbch_msg.crc ? "OK" : "KO");

    // unpack MIB
    srsran_mib_nr_t mib = {};
    if (srsran_pbch_msg_nr_mib_unpack(&search_res.pbch_msg, &mib) <
        SRSRAN_SUCCESS) {
      printf("Error unpacking PBCH-MIB\n");
    }

    char mib_info[512] = {};
    srsran_pbch_msg_nr_mib_info(&mib, mib_info, sizeof(mib_info));
    printf("MIB - %s\n", mib_info);
  }

  return SRSRAN_SUCCESS;
}
