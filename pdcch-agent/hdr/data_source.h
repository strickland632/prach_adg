#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <srsran/phy/io/filesource.h>
#include <srsran/phy/io/format.h>
#include <srsran/phy/rf/rf.h>
#include <string.h>

class data_source {
public:
  data_source(std::string file_path, srsran_datatype_t datatype);
  data_source(std::string rf_args, double rf_gain, double rf_freq);
  ~data_source();

private:
  srsran_filesource_t file_src;
  srsran_rf_t radio_src;
};

#endif // DATA_SOURCE_H
