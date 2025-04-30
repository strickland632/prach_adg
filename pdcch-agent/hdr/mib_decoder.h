#include "config.h"
#include "logging.h"
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
#include <thread>

class mib_decoder {
public:
  bool init(const agent_config_t &conf);
  bool decode_mib(cf_t *buffer, uint32_t sf_len);
  mib_decoder();
  ~mib_decoder();

private:
  // bool is_intialized;
  //  std::thread mib_thread;
  srsran_ssb_t ssb;
  srsran_ssb_args_t ssb_args;
  srsran_ssb_cfg_t ssb_cfg;
  uint32_t N_id;
};
