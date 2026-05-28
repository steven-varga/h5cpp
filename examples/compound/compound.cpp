// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada

#include <armadillo>
#include "pod.h"
#include "non-pod.h"
#include <h5cpp/all>
#include "generated.h"

#define CHUNK_SIZE 5
#define NROWS (4 * CHUNK_SIZE)
#define NCOLS (1 * CHUNK_SIZE)

int main() {
	h5::fd_t fd = h5::create("compound.h5", H5F_ACC_TRUNC);
	
	{ // Create a dataset of structs with chunking and GZIP compression, ready for partial I/O.
		h5::create<sn::example::record_t>(fd, "/orm/chunked_2D",
			h5::current_dims{NROWS, NCOLS}, h5::chunk{1, CHUNK_SIZE} | h5::gzip{8});
		h5::create<sn::typecheck::record_t>(fd, "/orm/typecheck", h5::max_dims{H5S_UNLIMITED});
	}
	{ // Create and write a vector of structs in a single call.
		std::vector<sn::example::record_t> records = h5::pod<sn::example::record_t>{} | h5::take(8);
		h5::write(fd, "orm/partial/vector one_shot", records);
		h5::write(fd, "orm/partial/vector custom_dims", records,
			h5::max_dims{H5S_UNLIMITED}, h5::gzip{9} | h5::chunk{20});
		h5::write(fd, "orm/partial/vector custom_dims different_order", records,
			h5::chunk{20} | h5::gzip{9}, h5::max_dims{H5S_UNLIMITED},
			h5::stride{6}, h5::block{4}, h5::current_dims{100}, h5::offset{2});
	}

	{ // Read the entire vector back.
		using T = std::vector<sn::example::record_t>;
		auto data = h5::read<T>(fd, "/orm/partial/vector one_shot");
		for (const auto& r : data)
			std::cerr << r.idx << " ";
		std::cerr << std::endl;
	}

	{ // Write and read back a compound dataset directly.
		std::vector<sn::example::record_t> records(NROWS * NCOLS);
		for (size_t i = 0; i < records.size(); ++i)
			records[i].idx = i;
		h5::write(fd, "/orm/chunked_2D", records,
				h5::current_dims{NROWS, NCOLS}, h5::chunk{1, CHUNK_SIZE} | h5::gzip{8});

		auto recs = h5::read<std::vector<sn::example::record_t>>(fd, "/orm/chunked_2D");
		std::cerr << "Read back " << recs.size() << " compound records." << std::endl;
	}

	{ // Tier-2 scatter/gather: non-POD fields (std::string, std::vector<double>)
		// The compiler generates scatter<T>/gather<T> specializations for structs
		// with vlen fields. Use h5::scatter / h5::gather directly (the declarations
		// have no definition; the compiler emits the body in generated.h).
		sn::sensor::timeseries_t ts;
		ts.timestamp_ns = 1'700'000'000'000'000'000ULL;
		ts.tag = "accelerometer";
		ts.internal_id = 42; // [[h5::ignore]] — not persisted
		ts.readings = {1.0, 2.0, 3.0, 4.0, 5.0};
		h5::scatter(fd, "/sensor/timeseries", ts);

		sn::sensor::timeseries_t ts_back;
		h5::gather(fd, "/sensor/timeseries", ts_back);
		std::cerr << "Tier-2 gather: timestamp=" << ts_back.timestamp_ns
			  << " label=" << ts_back.tag
			  << " readings=[";
		for (size_t i = 0; i < ts_back.readings.size(); ++i)
			std::cerr << (i ? "," : "") << ts_back.readings[i];
		std::cerr << "]" << std::endl;
	}
}
