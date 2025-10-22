// #pragma once

// #include "rf_base.h"
// #include "config.h"
// #include <srsran/phy/rf/rf.h>
// #include <memory>
// #include <string>

// class RF : public RFBase {
// public:
//     RF(const spoofer_config_t& config);
//     ~RF();

//     bool receive(std::complex<float>* buffer, uint32_t nsamples) override;
//     bool transmit(const std::complex<float>* buffer, uint32_t nsamples,
//                  bool start_of_burst = false, bool end_of_burst = false) override;

// private:
//     srsran_rf_t rf_device;
//     void configure_device(const spoofer_config_t& config);
//     std::string device_args;
// };