#pragma once
#include <string>
#include <vector>

namespace sn::sensor {
	// Tier-2 struct: compiler generates scatter/gather specializations for
	// non-POD fields (std::string and std::vector<T>) when T is scalar.
	struct [[h5::doc("Time-series sensor reading with variable-length fields"),
			h5::chunk(128), h5::compress("gzip", 6)]] timeseries_t {
		unsigned long long timestamp_ns;
		[[h5::name("label")]] std::string tag;
		[[h5::ignore]] int internal_id;          // not persisted to HDF5
		std::vector<double> readings;
	};
}
