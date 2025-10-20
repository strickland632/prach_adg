#ifndef FILE_SOURCE_H
#define FILE_SOURCE_H

#include "source.h"
//#include <zmq.hpp>

class FileSource : public Source {
public:
  //source_error_t collect_iq_data(const all_args_t& args) override;
  source_error_t create(YAML::Node rf_config) override;
  source_error_t recv(cf_t* buffer, size_t nof_samples) override;
  source_error_t send(cf_t* buffer, size_t nof_samples) override;

private:
  //zmq::context_t context;
  //zmq::socket_t socket;
};

#endif // !FILE_SOURCE_H
