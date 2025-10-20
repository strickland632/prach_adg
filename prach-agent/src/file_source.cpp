#include "file_source.h"

source_error_t create(YAML::Node rf_config) {
	// TODO: implement
	source_error_t res = {};
	res.msg = "Not implemented";
	res.type = SOURCE_NOT_IMPLEMENTED;
	return res;
}

source_error_t recv(cf_t* buffer, size_t nof_samples) {
	// TODO: implement
	source_error_t res = {};
	res.msg = "Not implemented";
	res.type = SOURCE_NOT_IMPLEMENTED;
	return res;
}

source_error_t send(cf_t* buffer, size_t nof_samples) {
	// TODO: implement
	source_error_t res = {};
	res.msg = "Not implemented";
	res.type = SOURCE_NOT_IMPLEMENTED;
	return res;
}
