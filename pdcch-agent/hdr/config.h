#ifndef CONFIG_H
#define CONFIG_H

#define FREQ_DEFAULT 627750000
#define SRATE_DEFAULT 23040000

#include "toml.hpp"

using namespace std;

typedef struct influxdb_config {
  bool metrics_influxdb_enable;
  std::string metrics_influxdb_url;
  uint32_t metrics_influxdb_port;
  std::string metrics_influxdb_org;
  std::string metrics_influxdb_token;
  std::string metrics_influxdb_bucket;
  float metrics_period_secs;
  std::string sniffer_data_identifier;
} influxdb_config_t;

typedef struct rf_config_s {
  std::string file_path;
  uint64_t sample_rate;
  double frequency;
  const char *rf_args;
} rf_config_t;

typedef struct agent_config_s {
  influxdb_config_t influx;
  rf_config_t rf;
} agent_config_t;

static agent_config_t load(std::string config_path) {
  printf("Loading config from path: %s\n", config_path.c_str());
  toml::table toml = toml::parse_file(config_path);
  agent_config_t conf;
  conf.rf.file_path = toml["rf"]["file_path"].value_or(""sv).data();
  conf.rf.sample_rate = toml["rf"]["sample_rate"].value_or(SRATE_DEFAULT);
  conf.rf.frequency = toml["rf"]["frequency"].value_or(FREQ_DEFAULT);
  conf.rf.rf_args = toml["rf"]["rf_args"].value_or("");

  conf.influx.metrics_influxdb_enable =
      toml["influxdb"]["metrics_influxdb_enable"].value_or(false);
  conf.influx.metrics_influxdb_url =
      toml["influxdb"]["metrics_influxdb_url"].value_or("");
  conf.influx.metrics_influxdb_port =
      toml["influxdb"]["metrics_influxdb_port"].value_or(0);
  conf.influx.metrics_influxdb_org =
      toml["influxdb"]["metrics_influxdb_org"].value_or("");
  conf.influx.metrics_influxdb_token =
      toml["influxdb"]["metrics_influxdb_token"].value_or("");
  conf.influx.metrics_influxdb_bucket =
      toml["influxdb"]["metrics_influxdb_bucket"].value_or("");
  conf.influx.metrics_period_secs =
      toml["influxdb"]["metrics_period_secs"].value_or(0.0);
  conf.influx.sniffer_data_identifier =
      toml["influxdb"]["sniffer_data_identifier"].value_or(false);

  // toml::array *pdcch_tables = toml["pdcch"].as<toml::array>();
  // for (toml::node &node : *pdcch_tables) {
  return conf;
}

#endif // CONFIG_H
