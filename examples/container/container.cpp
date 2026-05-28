// Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada

#include <h5cpp/all>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <array>
#include <iostream>

int main() {
	auto fd = h5::create("containers.h5", H5F_ACC_TRUNC);

	// ── contiguous sequences ─────────────────────────────────────────────────
	// vector<T>: direct pointer, single H5Dwrite call.
	// deque<T>, list<T>: copied to staging buffer, then written — same on-disk layout.
	// vector<array<T,N>>: flattened to T*, produces an N-column 2D dataset.
	{
		h5::gr_t gr = h5::gcreate(fd, "sequence");

		auto data = h5::uniform<int>{1, 100} | h5::take(20);
		h5::write(fd, "sequence/vector", data);
		h5::write(fd, "sequence/deque", std::deque<int>(data.begin(), data.end()));
		h5::write(fd, "sequence/list", std::list<int>(data.begin(), data.end()));

		std::vector<std::array<int,4>> rows(5);
		for (auto& row : rows)
			for (auto& v : row) v = *h5::uniform<int>{1, 100};
		h5::write(fd, "sequence/vec_array4", rows);

		auto v2 = h5::read<std::vector<int>>(fd, "sequence/vector");
		auto d2 = h5::read<std::deque<int>>(fd, "sequence/deque");
		auto l2 = h5::read<std::list<int>>(fd, "sequence/list");
		auto m2 = h5::read<std::vector<std::array<int,4>>>(fd, "sequence/vec_array4");

		// h5cpp's STL pretty-printer (H5Uall.hpp) inserts these directly into
		// std::ostream — no manual iteration. Long containers truncate at
		// H5CPP_CONSOLE_WIDTH (default 10) with a trailing ", ...".
		std::cout << "vector:        " << v2 << "\n";
		std::cout << "deque:         " << d2 << "\n";
		std::cout << "list:          " << l2 << "\n";
		std::cout << "vec_array4:    " << m2 << "  (each inner array prints recursively)\n";
	}

	// ── sorted sets ──────────────────────────────────────────────────────────
	// set<T>:      deduplicates; elements stored in ascending key order.
	// multiset<T>: retains duplicates; still sorted on disk.
	{
		h5::gr_t gr = h5::gcreate(fd, "sorted_set");

		std::vector<int> src{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};  // 11 elements, 7 unique
		h5::write(fd, "sorted_set/set",      std::set<int>     (src.begin(), src.end()));
		h5::write(fd, "sorted_set/multiset", std::multiset<int>(src.begin(), src.end()));

		auto s2  = h5::read<std::set<int>>     (fd, "sorted_set/set");
		auto ms2 = h5::read<std::multiset<int>>(fd, "sorted_set/multiset");

		std::cout << "set:           " << s2  << "  (size " << s2.size()  << ", unique)\n";
		std::cout << "multiset:      " << ms2 << "  (size " << ms2.size() << ", dups kept)\n";
	}

	// ── hash sets ────────────────────────────────────────────────────────────
	// unordered_set<T>:      O(1) lookup, no order guarantee, deduplicates.
	// unordered_multiset<T>: same but retains duplicates.
	{
		h5::gr_t gr = h5::gcreate(fd, "hash_set");

		std::vector<int> src{3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5};
		h5::write(fd, "hash_set/unordered_set",
			std::unordered_set<int>(src.begin(), src.end()));
		h5::write(fd, "hash_set/unordered_multiset",
			std::unordered_multiset<int>(src.begin(), src.end()));

		auto us2  = h5::read<std::unordered_set<int>>     (fd, "hash_set/unordered_set");
		auto ums2 = h5::read<std::unordered_multiset<int>>(fd, "hash_set/unordered_multiset");

		std::cout << "unordered_set: " << us2  << "  (size " << us2.size()  << ", unique)\n";
		std::cout << "u_multiset:    " << ums2 << "  (size " << ums2.size() << ", dups kept)\n";
	}

	// ── streaming / packet-table ─────────────────────────────────────────────
	// forward_list<T> has no .size() and no contiguous .data() — not usable with h5::write.
	// Pattern: unlimited chunked dataset + h5::append; read back as vector<T>.
	{
		h5::gr_t gr = h5::gcreate(fd, "streaming");

		std::forward_list<int> fl{7, 14, 21, 28, 35};
		h5::pt_t pt = h5::create<int>(fd, "streaming/forward_list",
			h5::max_dims{H5S_UNLIMITED}, h5::chunk{1});
		h5::append(pt, fl);

		auto back = h5::read<std::vector<int>>(fd, "streaming/forward_list");
		std::cout << "forward_list: " << back << "  (read back as vector<int>)\n";
	}

	// ── key-value containers ────────────────────────────────────────────────
	// map<K,V> / multimap<K,V> / unordered_map<K,V> / unordered_multimap<K,V>:  stored as a rank-1 dataset of an H5T_COMPOUND { key, value } (kv_t).
	// Sorted on key for map/multimap; bucket order for unordered_*. Round-trip reconstructs the original associative container.
	{
		h5::gr_t gr = h5::gcreate(fd, "key_value");

		std::map<int, double>           m   = {{1, 1.5}, {2, 2.5}, {3, 3.5}};
		std::multimap<short, int>       mm  = {{1, 10}, {1, 11}, {2, 20}};
		std::unordered_map<int, float>  um  = {{1, 1.5f}, {2, 2.5f}};
		std::unordered_multimap<int,int> umm = {{1, 10}, {1, 11}, {2, 20}};

		h5::write(fd, "key_value/map",                m);
		h5::write(fd, "key_value/multimap",           mm);
		h5::write(fd, "key_value/unordered_map",      um);
		h5::write(fd, "key_value/unordered_multimap", umm);

		auto m2   = h5::read<std::map<int, double>>          (fd, "key_value/map");
		auto mm2  = h5::read<std::multimap<short, int>>      (fd, "key_value/multimap");
		auto um2  = h5::read<std::unordered_map<int, float>> (fd, "key_value/unordered_map");
		auto umm2 = h5::read<std::unordered_multimap<int,int>>(fd, "key_value/unordered_multimap");

		std::cout << "map:           " << m2   << "  (sorted by key)\n";
		std::cout << "multimap:      " << mm2  << "  (sorted, dups kept)\n";
		std::cout << "unordered_map: " << um2  << "  (bucket order)\n";
		std::cout << "u_multimap:    " << umm2 << "  (bucket order, dups kept)\n";
	}

	// ── tuple / pair element types ──────────────────────────────────────────
	// vector<pair<K,V>>:  K and V trivially copyable → contiguous, written as  compound { first, second } via dt_t<pair>+offsetof.
	// vector<tuple<...>>: composite kind → each element is packed into a  C-struct mirror via traits::pack, then written as
	//                     a rank-1 dataset of H5T_COMPOUND.
	{
		h5::gr_t gr = h5::gcreate(fd, "tuple_pair");
		std::vector<std::pair<int, double>> vp = {{1, 1.5}, {2, 2.5}, {3, 3.5}};
		std::vector<std::tuple<int, double, float>> vt = {{1, 1.5, 2.5f}, {2, 2.5, 3.5f}, {3, 3.5, 4.5f}};

		h5::write(fd, "tuple_pair/vec_pair",  vp);
		h5::write(fd, "tuple_pair/vec_tuple", vt);

		auto vp2 = h5::read<std::vector<std::pair<int, double>>> (fd, "tuple_pair/vec_pair");
		auto vt2 = h5::read<std::vector<std::tuple<int, double, float>>>(fd, "tuple_pair/vec_tuple");

		std::cout << "vec<pair>:     " << vp2 << "  (compound {first,second})\n";
		std::cout << "vec<tuple>:    " << vt2 << "  (compound, packed per element)\n";
	}

	// ── variable-length sequences (vlen storage) ────────────────────────────
	// vector<string>:     each element is a separately-sized null-terminated
	//                     string; written as rank-1 of H5T_VARIABLE via a
	//                     char*-relay array (HDF5 owns no memory here).
	// vector<vector<T>>:  ragged inner extents per row; written as rank-1 of
	//                     H5Tvlen_create(base_type) via an hvl_t-relay array.
	// Both round-trip cleanly — read back reallocates the inner storage.
	{
		h5::gr_t gr = h5::gcreate(fd, "vlen");

		std::vector<std::string>      names  = {"alpha", "beta", "gamma", "delta", "epsilon"};
		std::vector<std::vector<int>> jagged = {{1, 2, 3}, {4, 5}, {6, 7, 8, 9}, {10}};

		h5::write(fd, "vlen/strings", names);
		h5::write(fd, "vlen/ragged",  jagged);

		auto names2  = h5::read<std::vector<std::string>>     (fd, "vlen/strings");
		auto jagged2 = h5::read<std::vector<std::vector<int>>>(fd, "vlen/ragged");

		std::cout << "vec<string>:   " << names2  << "  (H5T_VARIABLE, char* relay)\n";
		std::cout << "vec<vec<int>>: " << jagged2 << "  (H5Tvlen_create, hvl_t relay)\n";
	}

	// ── still not supported ─────────────────────────────────────────────────
	// Deeper nesting (vector<list<map<K,V>>>, vector<set<T>>, …) fires the
	// nested-container static_assert at compile time. vector<bool> is also
	// rejected (no contiguous bool*). See README "Not Yet Supported".
}
