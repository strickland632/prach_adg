#include "config.h"
#include "data_source.h"
#include "logging.h"
#include "mib_decoder.h"
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

int main(int argc, char **argv) {
  // MIB decoder thread
  // SSB decoder thread
  // PDCCH handler
  // Pass cell info
  // generic reader class
  // Open rf dev

  std::string config_path(argv[1]);
  agent_config_t conf = load(config_path);

  uint32_t sf_len = SRSRAN_SF_LEN_PRB(conf.rf.nof_prb);
  cf_t *buffer = srsran_vec_cf_malloc(sf_len);

  if (argc != 2) {
    LOG_ERROR("Usage: pdcch-agent <config file>\n");
    return EXIT_FAILURE;
  }

  data_source *src;
  if (!conf.rf.file_path.empty())
    src = new data_source(strdup(conf.rf.file_path.c_str()),
                          SRSRAN_COMPLEX_FLOAT_BIN);
  else
    src = new data_source(strdup(conf.rf.rf_args), conf.rf.gain,
                          conf.rf.frequency, conf.rf.sample_rate);

  mib_decoder mib_d;
  mib_d.init(conf);

  while (src->read(buffer, sf_len))
    mib_d.decode_mib(buffer, sf_len);

  return SRSRAN_SUCCESS;
}
