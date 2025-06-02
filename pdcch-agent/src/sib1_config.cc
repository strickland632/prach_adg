#include "sib1_config.h"

bool derive_sib1_config(const srsran_mib_nr_t& mib,
                        const srsran_carrier_nr_t& carrier,
                        srsran_coreset_t& coreset0,
                        srsran_search_space_t& search_space0) {
  
if (srsran_coreset_get_sib1_config(&mib, &coreset0) < SRSRAN_SUCCESS) {
    return false;
  }

  if (srsran_search_space_init_sib1(&carrier, &coreset0, &search_space0) <
      SRSRAN_SUCCESS) {
    return false;
  }

  return true;
}