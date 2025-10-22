#ifndef UHD_SOURCE_H
#define UHD_SOURCE_H

#include "source.h"
#include <uhd/stream.hpp>
#include <uhd/usrp/multi_usrp.hpp>

class UHDSource : public Source {
public:
	source_error_t create(YAML::Node rf_config) override;
	source_error_t recv(cf_t_1* buffer, size_t nof_samples) override;
	source_error_t send(cf_t_1* buffer, size_t nof_samples) override;

private:
	uhd::usrp::multi_usrp::sptr usrp;
};

#endif  // !UHD_SOURCE_H
