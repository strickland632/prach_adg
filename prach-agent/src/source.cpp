#include "source.h"
#include "uhd_source.h"
#include "zmq_source.h"
#include "file_source.h"

std::unique_ptr<Source> create_source_instance(const std::string& type) {
  if (type == "uhd") {
    return std::make_unique<UHDSource>();
  } else if (type == "zmq") {
    return std::make_unique<ZMQSource>();
  } else if (type == "file") {
		return std::make_unique<FileSource>();
  } else {
    throw std::runtime_error("Unknown/Unsupported source type:" + type + "\n");
  }
}
//change this template to go with my individual yaml stuff?
template<typename T> 
T validate(const YAML::Node node, const std::string key){
    if (!node[key]) {
        throw std::runtime_error("Missing required key: '" + key + "'");
    }

    try {
        return node[key].as<T>();
    } catch (const YAML::TypedBadConversion<T>&) {
        throw std::runtime_error("Key '" + key + "' is not of the expected type.");
    }
}
