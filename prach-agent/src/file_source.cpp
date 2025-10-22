#include "file_source.h"

source_error_t FileSource::create(YAML::Node rf_config) {
	// TODO: implement
	source_error_t res = {};
	res.msg = "Not implemented";
	res.type = SOURCE_NOT_IMPLEMENTED;
	return res;
}

source_error_t FileSource::recv(cf_t_1* buffer, size_t nof_samples) {
	// TODO: implement
	source_error_t res = {};
	res.msg = "Not implemented";
	res.type = SOURCE_NOT_IMPLEMENTED;
	return res;
}

source_error_t FileSource::send(cf_t_1* buffer, size_t nof_samples) {
	// TODO: implement
	source_error_t res = {};
	res.msg = "Not implemented";
	res.type = SOURCE_NOT_IMPLEMENTED;
	return res;
}
