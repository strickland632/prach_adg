#include "config.h"
#include "data_source.h"
#include "logging.h"
#include "mib_decoder.h"
#include "srsran/phy/sync/ssb.h"
#include <chrono>
#include <complex.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <srsran/phy/common/phy_common.h>
#include <srsran/phy/common/phy_common_nr.h>
#include <srsran/phy/phch/dci.h>
#include <srsran/phy/phch/dci_nr.h>
#include <srsran/phy/phch/pbch_msg_nr.h>
#include <srsran/phy/phch/pdcch_nr.h>
#include <srsran/phy/utils/vector.h>
#include <stdlib.h>
#include <sys/types.h>
#include <thread>
#include "sib1_config.h"

int main(int argc, char **argv) {
  // MIB decoder thread
  // SSB decoder thread
  // PDCCH handler
  // Pass cell info
  // generic reader class
  // Open rf dev

  std::string config_path(argv[1]);
  agent_config_t conf = load(config_path);

  uint32_t grid_size = conf.rf.nof_prb * SRSRAN_NRE * SRSRAN_NSYMB_PER_SLOT_NR;

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
  mib_d.init(conf, grid_size);
  mib_d.start_thread();

  srsran_pdcch_nr_args_t args = {};
  args.disable_simd = false;
  args.measure_evm = true;
  args.measure_time = true;

  srsran_pdcch_nr_t pdcch_rx = {};
  static srsran_carrier_nr_t carrier = {};

  carrier.pci = 1;
  carrier.dl_center_frequency_hz = conf.rf.frequency;
  carrier.ul_center_frequency_hz = conf.rf.frequency;
  carrier.ssb_center_freq_hz = conf.rf.frequency;
  carrier.offset_to_carrier = 0;
  carrier.nof_prb = conf.rf.nof_prb;
  carrier.start = 0;

  srsran_dmrs_pdcch_ce_t *ce = SRSRAN_MEM_ALLOC(srsran_dmrs_pdcch_ce_t, 1);
  SRSRAN_MEM_ZERO(ce, srsran_dmrs_pdcch_ce_t, 1);

  srsran_coreset_t coreset = {};
  if (conf.pdcch.interleaved) {
    coreset.mapping_type = srsran_coreset_mapping_type_interleaved;
    coreset.reg_bundle_size = srsran_coreset_bundle_size_n6;
    coreset.interleaver_size = srsran_coreset_bundle_size_n2;
    coreset.precoder_granularity =
        srsran_coreset_precoder_granularity_reg_bundle;
    coreset.shift_index = carrier.pci;

    carrier.pci = 500;
  }

  if (srsran_pdcch_nr_init_rx(&pdcch_rx, &args) < SRSRAN_SUCCESS) {
    ERROR("Error init");
    return EXIT_FAILURE;
  }

  uint32_t nof_frequency_resource =
      SRSRAN_MIN(SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE, carrier.nof_prb / 6);

  cf_t *buffer = srsran_vec_cf_malloc(grid_size);
  size_t idx = 0;
  while (src->read(buffer, grid_size)) {
    mib_d.add_frame_to_queue(buffer);
    idx++;

    srsran_mib_nr_t latest = mib_d.request_mib();
    if (idx % 1000 != 0 && idx > 0) {
      buffer = srsran_vec_cf_malloc(grid_size);
      continue;
    }
    carrier.scs = latest.scs_common;

    srsran_coreset_t coreest0 = {};
    srsran_search_space_t search_space0 = {};

    if (!derive_sib1_config(latest, carrier, corset0, search_space0)) {
      LOG_ERROR ("Failed to derive CORESET0 or SearchSpace0");
      continue;
    }

    if (srsran_pdcch_nr_set_carrier(&pdcch_rx, &carrier, &corset0) < SRSRAN_SUCCESS) {
      LOG_ERROR("Failed to set carrier for CORESET0")
      continue;
    }

    for (uint32_t agg = 0; agg < SRSRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR; agg++){
      search_space0.nof_candidates[agg] = srsran_pdcch_nr_max_candidates_coreset(&coreset0, agg);

    }

    """
    for (uint32_t frequency_resources = 1;
         frequency_resources < (1U << nof_frequency_resource);
         frequency_resources = ((frequency_resources << 1U) | 1U)) {
      for (uint32_t i = 0; i < nof_frequency_resource; i++) {
        uint32_t mask = ((frequency_resources >> i) & 1U);
        coreset.freq_resources[i] = (mask == 1);
      }
    
      for (coreset.duration = SRSRAN_CORESET_DURATION_MIN;
           coreset.duration <= SRSRAN_CORESET_DURATION_MAX;
           coreset.duration++) {
        """
        uint32_t N = srsran_coreset_get_bw(&coreset) * coreset.duration;
        if (conf.pdcch.interleaved && N % 12 != 0) {
          continue;
        }

        srsran_search_space_t search_space = {};
        search_space.type = srsran_search_space_type_ue;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_0_0;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_1_0;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_0_1;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_0_1;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_1_1;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_2_0;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_2_1;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_2_2;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_2_3;
        search_space.formats[search_space.nof_formats++] =
            srsran_dci_format_nr_rar;

        srsran_dci_cfg_nr_t dci_cfg = {};
        dci_cfg.coreset0_bw = 0;
        dci_cfg.bwp_dl_initial_bw = carrier.nof_prb;
        dci_cfg.bwp_dl_active_bw = carrier.nof_prb;
        dci_cfg.bwp_ul_initial_bw = carrier.nof_prb;
        dci_cfg.bwp_ul_active_bw = carrier.nof_prb;
        dci_cfg.monitor_common_0_0 = true;
        dci_cfg.monitor_0_0_and_1_0 = true;
        dci_cfg.monitor_0_1_and_1_1 = true;

        srsran_dci_nr_t dci = {};
        if (srsran_dci_nr_set_cfg(&dci, &dci_cfg) < SRSRAN_SUCCESS) {
          LOG_ERROR("Error setting DCI configuratio");
          continue;
        }

        if (srsran_pdcch_nr_set_carrier(&pdcch_rx, &carrier, &coreset) <
            SRSRAN_SUCCESS) {
          LOG_ERROR("Error setting carrier");
          continue;
        }

        for (uint32_t aggregation_level = 0;
             aggregation_level < SRSRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR;
             aggregation_level++) {
          search_space.nof_candidates[aggregation_level] =
              srsran_pdcch_nr_max_candidates_coreset(&coreset,
                                                     aggregation_level);
        }

        for (uint32_t aggregation_level = 0;
             aggregation_level < SRSRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR;
             aggregation_level++) {
          uint32_t L = 1U << aggregation_level;

          for (uint32_t slot_idx = 0;
               slot_idx < SRSRAN_NSLOTS_PER_FRAME_NR(carrier.scs); slot_idx++) {
            uint32_t dci_locations[SRSRAN_SEARCH_SPACE_MAX_NOF_CANDIDATES_NR] =
                {};

            // Calculate candidate locations
            int n = srsran_pdcch_nr_locations_coreset(
                &coreset, &search_space, conf.pdcch.rnti, aggregation_level,
                slot_idx, dci_locations);
            if (n < SRSRAN_SUCCESS) {
              ERROR("Error calculating locations in CORESET");
              continue;
            }

            // Skip if no candidates
            if (n == 0) {
              continue;
            }

            for (uint32_t ncce_idx = 0; ncce_idx < n; ncce_idx++) {

              ce->nof_re = (SRSRAN_NRE - 3) * 6 * L;
              for (uint32_t i = 0; i < SRSRAN_PDCCH_MAX_RE; i++) {
                ce->ce[i] = (i < ce->nof_re) ? 1.0f : 0.0f;
              }
              ce->noise_var = 0.0f;

              srsran_dci_msg_nr_t dci_msg_rx = {};
              dci_msg_rx.ctx.format = srsran_dci_format_nr_1_0;
              dci_msg_rx.ctx.location.L = aggregation_level;
              dci_msg_rx.ctx.location.ncce = dci_locations[ncce_idx];
              dci_msg_rx.nof_bits = srsran_dci_nr_size(
                  &dci, search_space.type, srsran_dci_format_nr_1_0);

              for (srsran_rnti_type_t test_type = srsran_rnti_type_c;
                   test_type <= srsran_rnti_type_mcs_c;
                   test_type = static_cast<srsran_rnti_type_t>(
                       static_cast<int>(test_type) + 1)) {
                dci_msg_rx.ctx.rnti_type = test_type;
                srsran_pdcch_nr_res_t res = {};
                srsran_vec_u8_zero(dci_msg_rx.payload, dci_msg_rx.nof_bits);
                if (srsran_pdcch_nr_decode(&pdcch_rx, buffer, ce, &dci_msg_rx,
                                           &res) == SRSRAN_SUCCESS) {

                  if (!res.crc) {
                    continue;
                  }
                  srsran_dci_dl_nr_t dci_rx = {};
                  if (srsran_dci_nr_dl_unpack(&dci, &dci_msg_rx, &dci_rx) !=
                      SRSRAN_SUCCESS) {
                    LOG_ERROR("Failed to unpack DCI");
                  }
                  char str[512];
                  if (srsran_dci_dl_nr_to_str(&dci, &dci_rx, str,
                                              (uint32_t)sizeof(str)) == 0) {
                    LOG_ERROR("Failed to get DCI string");
                  }
                  LOG_INFO("GOT DCI: %s", str);
                }
              }
            }
          }
        }
      }
    }

    buffer = srsran_vec_cf_malloc(grid_size);
  }

  mib_d.stop_thread();

  return SRSRAN_SUCCESS;
}
