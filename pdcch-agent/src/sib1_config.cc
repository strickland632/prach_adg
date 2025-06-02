#include "sib1_config.h"
#include <srsran/phy/phch/pdcch_nr.h>
#include <srsran/phy/common/phy_common_nr.h>

bool derive_sib1_config(const srsran_mib_nr_t& mib,
                        const srsran_carrier_nr_t& carrier,
                        srsran_coreset_t& coreset0,
                        srsran_search_space_t& search_space0)
{
  // Hardcoded default 5G NR CORESET0 (as per MIB spec & 15kHz SCS grid assumptions)
  coreset0.duration = 2;
  coreset0.freq_resources[0] = true;
  for (int i = 1; i < SRSRAN_CORESET_FREQ_DOMAIN_RES_SIZE; i++)
    coreset0.freq_resources[i] = false;
  coreset0.mapping_type = srsran_coreset_mapping_type_non_interleaved;
  coreset0.reg_bundle_size = srsran_coreset_bundle_size_n6;
  coreset0.interleaver_size = srsran_coreset_bundle_size_n2;
  coreset0.precoder_granularity = srsran_coreset_precoder_granularity_reg_bundle;
  coreset0.shift_index = carrier.pci;

  // Search Space 0 configuration (SI-RNTI, DCI 1_0, Aggregation Levels 4-16)
  search_space0.type = srsran_search_space_type_common;
  search_space0.monitoring_slot_period = 1;
  search_space0.first_symbol = 0;
  search_space0.duration = 2;
  search_space0.coreset_id = 0;
  search_space0.nof_formats = 1;
  search_space0.formats[0] = srsran_dci_format_nr_1_0;

  for (int i = 0; i < SRSRAN_SEARCH_SPACE_NOF_AGGREGATION_LEVELS_NR; i++)
    search_space0.nof_candidates[i] = 0;

  // Enable AL-4, AL-8, AL-16 (as allowed in default SIB1 monitoring)
  search_space0.nof_candidates[2] = 4; // AL=4
  search_space0.nof_candidates[3] = 2; // AL=8
  search_space0.nof_candidates[4] = 1; // AL=16

  return true;
}
