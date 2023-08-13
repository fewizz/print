#pragma once

#include "./buffer.hpp"

namespace print {

	inline buffer<1024> out{ posix::std_out };
	inline buffer<1> err{ posix::std_err };

}