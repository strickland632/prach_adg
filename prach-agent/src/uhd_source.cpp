#include "uhd_source.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>



source_error_t UHDSource::create(YAML::Node rf_config) {
	source_error_t res = {}; //init struct to return to main? or something?

	try {
		// Create USRP
		usrp = uhd::usrp::multi_usrp::make(validate<std::string>(rf_config, "rf_args"));

		usrp->set_rx_rate(validate<double>(rf_config, "srate"));
		usrp->set_rx_freq(uhd::tune_request_t(validate<double>(rf_config, "freq")));
		usrp->set_rx_gain(validate<double>(rf_config, "gain"));

		res.msg = "";
		res.type = SOURCE_SUCCESS;
  } catch (const std::exception& e) {
		res.msg = e.what();
		res.type = SOURCE_UHD_ERROR;
	}

	return res;
}

// Collect IQ data using UHD
source_error_t UHDSource::recv(cf_t_1* buffer,  size_t nof_samples) {
  source_error_t res = {};

  try {
    uhd::stream_args_t stream_args("fc32", "sc16");
    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
    const size_t samps_per_buff = rx_stream->get_max_num_samps();

    std::vector<cf_t_1> tmp(samps_per_buff);
    uhd::rx_metadata_t md;

    uhd::stream_cmd_t stream_cmd(
        uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps  = nof_samples;
    stream_cmd.stream_now = true;
    rx_stream->issue_stream_cmd(stream_cmd);

    size_t total_received = 0;
    while (total_received < nof_samples) {
      size_t num_to_recv = std::min(samps_per_buff, nof_samples - total_received);
      size_t n = rx_stream->recv(tmp.data(), num_to_recv, md, 3.0);

      if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
        res.msg = md.strerror();
	res.type = SOURCE_UHD_ERROR;
        return res;
      }

      std::copy(tmp.begin(), tmp.begin() + n, buffer + total_received);
      total_received += n;
    }

    res.msg = "";
    res.type = SOURCE_SUCCESS;
    return res;
  } catch (const std::exception& e) {
    res.msg = e.what();
    res.type = SOURCE_UHD_ERROR;
    return res;
  }
}
