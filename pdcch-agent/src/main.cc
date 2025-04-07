#include "config.h"
#include "filesource.h"
#include "srsran/phy/sync/ssb.h"
#include <complex.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <srsran/phy/common/phy_common.h>
#include <srsran/phy/utils/vector.h>
#include <stdlib.h>
#include <sys/types.h>

#define SSB_NOF_SAMPLES 10000

static srsran_ssb_pattern_t ssb_pattern = SRSRAN_SSB_PATTERN_C;
static srsran_subcarrier_spacing_t ssb_scs = srsran_subcarrier_spacing_30kHz;
static srsran_duplex_mode_t duplex_mode = SRSRAN_DUPLEX_MODE_TDD;

int main(int argc, char **argv) {
  // generic reader class
  // Open rf dev
  // search for ssb
  // decode pbch
  // unpack mib
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
    // Perform SSB-CSI Search
    srsran_ssb_search_res_t res = {};
    if (srsran_ssb_search(&ssb, buffer, SSB_NOF_SAMPLES, &res) <
        SRSRAN_SUCCESS) {
      printf("Error performing SSB-CSI search\n");
    }
  }

  return SRSRAN_SUCCESS;
}
