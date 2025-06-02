#ifndef SIB1_CONFIG_H
#define SIB1_CONFIG_H

#include <srsran/phy/phch/pdcch_nr.h>
#include <srsran/phy/phch/dci_nr.h>
#include <srsran/phy/sync/ssb.h>

bool derive_sib1_config(const srsran_mib_nr_t& mib,
                        const srsran_carrier_nr_t& carrier,
                        srsran_coreset_t& coreset0,
                        srsran_search_space_t& search_space0);

#endif // SIB1_CONFIG_H
