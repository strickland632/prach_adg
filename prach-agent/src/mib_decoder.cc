#include "mib_decoder.h"
#include <cstdlib>
#include <mutex>
#include <srsran/phy/phch/pbch_msg_nr.h>

mib_decoder::mib_decoder() : subframe_queue(20), decoder_running(false) {
  latest_mib = {};
}

mib_decoder::~mib_decoder() {}

bool mib_decoder::init(const agent_config_t &conf, uint32_t frame_len) {
  ssb = {};
  ssb_args = {};
  ssb_args.enable_decode = true;
  ssb_args.enable_search = true;
  if (srsran_ssb_init(&ssb, &ssb_args) < SRSRAN_SUCCESS) {
    LOG_ERROR("ssb init failed");
    return false;
  }

  ssb_cfg = {};
  ssb_cfg.srate_hz = conf.rf.sample_rate;
  ssb_cfg.center_freq_hz = conf.rf.frequency;
  ssb_cfg.ssb_freq_hz = conf.rf.frequency;
  ssb_cfg.scs = conf.ssb.ssb_scs;
  ssb_cfg.pattern = conf.ssb.ssb_pattern;
  ssb_cfg.duplex_mode = conf.ssb.duplex_mode;
  if (srsran_ssb_set_cfg(&ssb, &ssb_cfg) < SRSRAN_SUCCESS) {
    LOG_ERROR("error setting SSB configuration");
    return false;
  }
  N_id = conf.rf.N_id;
  sf_len = frame_len;
  decoder_running = true;
  return true;
}

bool mib_decoder::decode_mib(cf_t *buffer) {
  srsran_csi_trs_measurements_t meas = {};
  if (srsran_ssb_csi_search(&ssb, buffer, sf_len, &N_id, &meas) <
      SRSRAN_SUCCESS) {
    LOG_ERROR("Error performing SSB-CSI search");
  }

  char str[512] = {};
  // srsran_csi_meas_info(&meas, str, sizeof(str));
  // if (meas.rsrp != 0)
  // LOG_DEBUG("CSI MEAS - search pci=%d %s", N_id, str);

  srsran_ssb_search_res_t search_res = {};
  if (srsran_ssb_search(&ssb, buffer, sf_len, &search_res) < SRSRAN_SUCCESS) {
    LOG_ERROR("Error performing SSB search");
  }

  srsran_pbch_msg_info(&search_res.pbch_msg, str, sizeof(str));
  if (!search_res.pbch_msg.crc)
    return true;
  LOG_DEBUG("SSB PBCH - t_offset=%d pci=%d %s crc=%s", search_res.t_offset,
            search_res.N_id, str, search_res.pbch_msg.crc ? "OK" : "KO");

  // unpack MIB
  srsran_mib_nr_t mib = {};
  if (srsran_pbch_msg_nr_mib_unpack(&search_res.pbch_msg, &mib) <
      SRSRAN_SUCCESS) {
    LOG_ERROR("Error unpacking PBCH-MIB");
  }

  char mib_info[512] = {};
  srsran_pbch_msg_nr_mib_info(&mib, mib_info, sizeof(mib_info));
  LOG_DEBUG("MIB - %s", mib_info);

  {
    std::lock_guard<std::mutex> lock(mib_mutex);
    latest_mib = mib;
  }

  return true;
}

srsran_mib_nr_t mib_decoder::request_mib() {
  std::lock_guard<std::mutex> lock(mib_mutex);
  return latest_mib;
}

bool mib_decoder::add_frame_to_queue(cf_t *buffer) {
  while (!subframe_queue.push(buffer)) {
    LOG_WARN("failed to push buffer to MIB frame queue");
  }
  return true;
}

void mib_decoder::run_decoder() {
  LOG_INFO("Starting MIB decoder");
  cf_t *buffer;
  bool got_buffer = false;
  while ((got_buffer = subframe_queue.pop(buffer)) || decoder_running) {
    if (!got_buffer) {
      std::this_thread::sleep_for(std::chrono::microseconds(1));
      continue;
    }
    try {
      decode_mib(buffer);
    } catch (const std::exception &e) {
      LOG_ERROR("Exception during decode_mib: %s", e.what());
    } catch (...) {
      LOG_ERROR("Unknown exception during decode_mib");
    }
    // free(buffer);
  }
  LOG_INFO("MIB decoder exiting...");
}

bool mib_decoder::start_thread() {
  decoder_thread = std::thread(&mib_decoder::run_decoder, this);
  return true;
}

void mib_decoder::stop_thread() { decoder_running = false; }
