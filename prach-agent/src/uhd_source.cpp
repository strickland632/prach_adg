// #include "uhd_source.h"
// #include <iostream>
// #include <fstream>
// #include <vector>
// #include <algorithm>






// source_error_t UHDSource::create(YAML::Node rf_config) {
//  source_error_t res = {}; //init struct to return to main? or something?


//  try {
//    // Create USRP
//    usrp = uhd::usrp::multi_usrp::make(validate<std::string>(rf_config, "rf_args"));


//    usrp->set_rx_rate(validate<double>(rf_config, "srate"));
//    usrp->set_rx_freq(uhd::tune_request_t(validate<double>(rf_config, "freq")));
//    usrp->set_rx_gain(validate<double>(rf_config, "gain"));


//    res.msg = "";
//    res.type = SOURCE_SUCCESS;
//  } catch (const std::exception& e) {
//    res.msg = e.what();
//    res.type = SOURCE_UHD_ERROR;
//  }


//  return res;
// }


// // Collect IQ data using UHD
// source_error_t UHDSource::recv(cf_t_1* buffer,  size_t nof_samples) {
//  source_error_t res = {};


//  try {
//    uhd::stream_args_t stream_args("fc32", "sc16");
//    uhd::rx_streamer::sptr rx_stream = usrp->get_rx_stream(stream_args);
//    const size_t samps_per_buff = rx_stream->get_max_num_samps();


//    std::vector<cf_t_1> tmp(samps_per_buff);
//    uhd::rx_metadata_t md;


//    uhd::stream_cmd_t stream_cmd(
//        uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
//    stream_cmd.num_samps  = nof_samples;
//    stream_cmd.stream_now = true;
//    rx_stream->issue_stream_cmd(stream_cmd);


//    size_t total_received = 0;
//    while (total_received < nof_samples) {
//      size_t num_to_recv = std::min(samps_per_buff, nof_samples - total_received);
//      size_t n = rx_stream->recv(tmp.data(), num_to_recv, md, 3.0);


//      if (md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
//        res.msg = md.strerror();
//  res.type = SOURCE_UHD_ERROR;
//        return res;
//      }


//      std::copy(tmp.begin(), tmp.begin() + n, buffer + total_received);
//      total_received += n;
//    }


//    res.msg = "";
//    res.type = SOURCE_SUCCESS;
//    return res;
//  } catch (const std::exception& e) {
//    res.msg = e.what();
//    res.type = SOURCE_UHD_ERROR;
//    return res;
//  }
// }




// // // Transmit existing buffer
// // // This does not generate samples; it only sends the samples already in `buffer`.
// // // The wire format is sc16, so UHD will convert

// // source_error_t UHDSource::send(const cf_t* buffer,
// //                               size_t nof_samples,
// //                               double tx_rate,
// //                               double center_freq_hz,
// //                               double tx_gain_db,
// //                               double seconds_in_future /* e.g., 0.050 */)
// // {
 
// //  // Prepare a default result object to return success or error info.
// //  source_error_t res = {};

// //  try {
   
// //    // Configure USRP TX chain: sample rate, center frequency, and gain.
// //    usrp->set_tx_rate(tx_rate);
// //    usrp->set_tx_freq(uhd::tune_request_t(center_freq_hz));
// //    usrp->set_tx_gain(tx_gain_db);
   
// //    // Create a TX streamer with host format fc32 (complex float) and wire format sc16.
// //    uhd::stream_args_t sargs("fc32", "sc16");
// //    uhd::tx_streamer::sptr tx = usrp->get_tx_stream(sargs);
   
// //    // Determine the maximum number of samples per packet the streamer can send (MTU).
// //    const size_t mtu = tx->get_max_num_samps();
   
// //    // Schedule the transmission to start a short time in the future for precise timing.
// //    const auto now     = usrp->get_time_now();
// //    const auto tx_time = now + uhd::time_spec_t(seconds_in_future);
   
// //    // Set up metadata for a timed, bursty transmission starting at tx_time.
// //    uhd::tx_metadata_t md{};
// //    md.has_time_spec  = true;
// //    md.time_spec      = tx_time;
// //    md.start_of_burst = true;
// //    md.end_of_burst   = false;

// //    // Send the buffer in chunks of size <= mtu until all samples are transmitted.
// //    size_t offset = 0;
// //    while (offset < nof_samples) {
// //      const size_t to_send = std::min(mtu, nof_samples - offset);
// //      const void*  chunk   = static_cast<const void*>(buffer + offset);
// //      const size_t sent    = tx->send(chunk, to_send, md);

// //      // Verify that UHD accepted the full chunk; otherwise return an error.
// //      if (sent != to_send) {
// //        res.msg  = "Short send on TX burst";
// //        res.type = SOURCE_UHD_ERROR;
// //        return res;
// //      }
     
// //      // After the first packet, clear start/time flags so following packets stream continuously.
// //      md.start_of_burst = false;
// //      md.has_time_spec  = false;

// //      offset += sent;
// //    }
   
// //    // Mark end of burst with a zero-length send so the USRP knows the burst is complete.
// //    md.end_of_burst = true;
// //    tx->send("", 0, md);
   
// //    // Return success if we reach here with no exceptions.
// //    res.msg  = "";
// //    res.type = SOURCE_SUCCESS;
// //    return res;

// //  } catch (const std::exception& e) {
   
// //    // On any UHD or std:: exception, capture the error message and return failure.
// //    res.msg  = e.what();
// //    res.type = SOURCE_UHD_ERROR;
// //    return res;
// //  }
// // }



