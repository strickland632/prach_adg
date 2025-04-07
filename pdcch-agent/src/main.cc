#include "srsran/phy/rf/rf.h"
#include <complex.h>
#include <cstring>
#include <pthread.h>
#include <srsran/phy/common/phy_common.h>
#include <srsran/phy/utils/vector.h>
#include <stdlib.h>

#define NOF_RX_ANT 1
#define NUM_SF (500)
#define SF_LEN (1920)
#define RF_BUFFER_SIZE (SF_LEN * NUM_SF)
#define TX_OFFSET_MS (4)

static cf_t agent_rx_buffer[NOF_RX_ANT][RF_BUFFER_SIZE];

static srsran_rf_t agent_radio;
pthread_t rx_thread;

int main() {
  char rf_args[RF_PARAM_LEN] = "rx_file=/home/charles/collected_iq/"
                               "pdcch_1842MHz.fc32,base_srate=23.04e6";
  // strncpy(rf_args, (char *)args, RF_PARAM_LEN - 1);
  if (srsran_rf_open_devname(&agent_radio, "file", rf_args, NOF_RX_ANT)) {
    fprintf(stderr, "Error opening rf\n");
    exit(-1);
  }

  // set the gain to 40dBm
  srsran_rf_set_rx_gain_ch(&agent_radio, 0, 40.0);

  // receive 5 subframes at once (i.e. mimic initial rx that receives one slot)
  uint32_t num_slots = NUM_SF / 5;
  uint32_t num_samps_per_slot = SF_LEN * 5;
  uint32_t num_rxed_samps = 0;
  for (uint32_t i = 0; i < num_slots; ++i) {
    void *data_ptr[SRSRAN_MAX_PORTS] = {NULL};
    for (uint32_t c = 0; c < NOF_RX_ANT; c++) {
      data_ptr[c] = &agent_rx_buffer[c][i * num_samps_per_slot];
    }
    num_rxed_samps += srsran_rf_recv_with_time_multi(
        &agent_radio, data_ptr, num_samps_per_slot, true, NULL, NULL);
  }

  printf("received %d samples.\n", num_rxed_samps);
  printf("closing ue rx device\n");

  srsran_rf_close(&agent_radio);

  return SRSRAN_SUCCESS;
}
