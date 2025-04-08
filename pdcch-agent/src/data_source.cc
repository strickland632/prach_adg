#include "data_source.h"
#include <srsran/phy/io/filesource.h>

data_source::data_source(std::string file_path, srsran_datatype_t datatype) {
  file_src = {};
  srsran_filesource_init(&file_src, file_path.c_str(), datatype);
}

data_source::data_source(std::string rf_args, double rf_gain, double srate,
                         double rf_freq) {
  if (srsran_rf_open(&radio_src, rf_args.c_str())) {
    fprintf(stderr, "Error opening rf\n");
    exit(-1);
  }

  srsran_rf_set_rx_gain(&radio_src, rf_gain);
  printf("Set RX rate: %.2f MHz\n",
         srsran_rf_set_rx_srate(&radio_src, srate) / 1000000);
  printf("Set RX gain: %.1f dB\n", srsran_rf_get_rx_gain(&radio_src));
  printf("Set RX freq: %.2f MHz\n",
         srsran_rf_set_rx_freq(&radio_src, 0, rf_freq) / 1000000);
}

data_source::~data_source() {
  if (&file_src != nullptr) {
    srsran_filesource_free(&file_src);
  }

  if (&radio_src != nullptr) {
    srsran_rf_close(&radio_src);
  }
}
