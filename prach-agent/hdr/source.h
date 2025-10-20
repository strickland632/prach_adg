#ifndef SOURCE_H
#define SOURCE_H

#include <memory>
#include <yaml-cpp/yaml.h>
#include <stdexcept>
#include <string>
#include <complex>

typedef std::complex<float> cf_t;

typedef enum source_error_type_e {
  SOURCE_SUCCESS = 0,
  SOURCE_FILE_ERROR,
  SOURCE_UHD_ERROR,
  SOURCE_ZMQ_ERROR,
  SOURCE_SAMPLE_ERROR,
  SOURCE_INVALID_SOURCE_TYPE,
  SOURCE_NOT_IMPLEMENTED
} source_error_type_t;

typedef struct source_error_s {
	std::string msg = "";
	source_error_type_e type = SOURCE_SUCCESS;
} source_error_t;

class Source {
  public:
    virtual ~Source() = default;
    virtual source_error_t create(YAML::Node rf_config);
    virtual source_error_t recv(cf_t* buffer, size_t nof_samples) = 0;
    virtual source_error_t send(cf_t* buffer, size_t nof_samples) = 0;
};

// SOURCE factory function
std::unique_ptr<Source> create_source_instance(const std::string& type);

template<typename T> 
T validate(const YAML::Node node, const std::string key);



#endif  // !SOURCE_H
