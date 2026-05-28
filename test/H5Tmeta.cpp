#include <hdf5.h>
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <h5cpp/compat.hpp>
#include <doctest/all>
#include <h5cpp/H5Tmeta.hpp>
#include <array>
#include <complex>
#include <cstddef>
#include <deque>
#include <forward_list>
#include <initializer_list>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>


template<class T>
using identity_t = T;

template<class... Ts>
using enable_or_op_t = h5::meta::enable_or<Ts...>;

template<class... Ts>
using enable_and_op_t = h5::meta::enable_and<Ts...>;
struct pod_value_t {
    int    x;
    double y;
};

struct non_pod_value_t {
    non_pod_value_t() {}
    int x;
};

struct iterable_value_t {
    using value_type      = int;
    using size_type       = std::size_t;
    using iterator        = int*;
    using const_iterator  = const int*;

    int* begin();
    int* end();
    const int* cbegin() const;
    const int* cend() const;
    int* data();
    std::size_t size() const;
};

struct sized_value_t {
    std::size_t size() const;
};

struct value_type_only_t {
    using value_type = int;
};

enum capability_enum_unscoped { capability_enum_unscoped_a, capability_enum_unscoped_b };
enum class capability_enum_scoped { a, b };

TEST_CASE("H5Tmeta is_array detects C arrays and std::array") {
    CHECK(h5::meta::is_array<int[4]>::value);
    CHECK(h5::meta::is_array<double[2][3]>::value);
    CHECK(h5::meta::is_array<std::array<int, 4>>::value);

    CHECK_FALSE(h5::meta::is_array<int>::value);
    CHECK_FALSE(h5::meta::is_array<std::vector<int>>::value);
}

TEST_CASE("H5Tmeta is_stl detects container-like types") {
    CHECK(h5::meta::is_stl<std::vector<int>>::value);
    CHECK(h5::meta::is_stl<std::string>::value);
    CHECK(h5::meta::is_stl<std::array<int, 4>>::value);
    CHECK(h5::meta::is_stl<iterable_value_t>::value);
    CHECK(h5::meta::is_stl<value_type_only_t>::value);
    CHECK(h5::meta::is_stl<sized_value_t>::value);

    CHECK_FALSE(h5::meta::is_stl<int>::value);
    CHECK_FALSE(h5::meta::is_stl<pod_value_t>::value);
}

template<class T> using identity_t = T;

TEST_CASE("H5Tmeta enable_or and enable_and participate correctly") {
    // On master, enable_or/enable_and alias std::enable_if<...>, not void directly.
    // The nested ::type member is void.
    CHECK((std::is_same_v<typename h5::meta::enable_or<std::true_type>::type, void>));
    CHECK((std::is_same_v<typename h5::meta::enable_or<std::false_type, std::true_type>::type, void>));
    CHECK((std::is_same_v<typename h5::meta::enable_and<std::true_type>::type, void>));
    CHECK((std::is_same_v<typename h5::meta::enable_and<std::true_type, std::true_type>::type, void>));

    // std::enable_if<false> is a valid (empty) type, so is_detected returns true.
    // These assertions document current master behavior; refined in #85-#90.
    CHECK((h5::meta::compat::is_detected_v<enable_or_op_t, std::false_type>));
    CHECK((h5::meta::compat::is_detected_v<enable_and_op_t, std::true_type, std::false_type>));
}

TEST_CASE("H5Tmeta decay strips containers to element type") {
    CHECK((std::is_same_v<typename h5::meta::decay<int>::type, int>));
    CHECK((std::is_same_v<typename h5::meta::decay<std::vector<int>>::type, int>));
    CHECK((std::is_same_v<typename h5::meta::decay<std::array<double, 3>>::type, double>));
    CHECK((std::is_same_v<typename h5::meta::decay<int[7]>::type, int>));
    CHECK((std::is_same_v<typename h5::meta::decay<double[2][3]>::type, double>));
}

TEST_CASE("H5Tmeta rank detects scalars arrays pointers and STL types") {
    CHECK(h5::meta::rank<int>::value == 0);
    CHECK(h5::meta::rank<double>::value == 0);

    CHECK(h5::meta::rank<int*>::value == 1);
    CHECK(h5::meta::rank<double*>::value == 1);

    CHECK(h5::meta::rank<int[3]>::value == 1);
    CHECK(h5::meta::rank<int[2][5]>::value == 2);
    CHECK(h5::meta::rank<int[2][3][4]>::value == 3);

    CHECK(h5::meta::rank<std::vector<int>>::value == 1);
    CHECK(h5::meta::rank<std::array<int, 3>>::value == 1);

    CHECK(h5::meta::rank<std::string>::value == 0);
    CHECK(h5::meta::rank<std::string_view>::value == 0);

    CHECK(h5::meta::rank<char[8]>::value == 0);
}

TEST_CASE("H5Tmeta rank aliases classify tensor depth") {
    CHECK(h5::meta::is_rank<int, 0>::value);
    CHECK(h5::meta::is_rank<int[3], 1>::value);
    CHECK(h5::meta::is_rank<int[2][3], 2>::value);
    CHECK(h5::meta::is_rank<int[2][3][4], 3>::value);

    CHECK(h5::meta::is_scalar<int>::value);
    CHECK(h5::meta::is_vector<int[3]>::value);
    CHECK(h5::meta::is_vector<std::vector<int>>::value);
    CHECK(h5::meta::is_matrix<int[2][3]>::value);
    CHECK(h5::meta::is_cube<int[2][3][4]>::value);

    CHECK_FALSE(h5::meta::is_scalar<std::vector<int>>::value);
    CHECK_FALSE(h5::meta::is_matrix<int[3]>::value);
    CHECK_FALSE(h5::meta::is_cube<int[2][3]>::value);
}

TEST_CASE("H5Tmeta is_string detects string and string_view") {
    CHECK(h5::meta::is_string<std::string>::value);
    CHECK(h5::meta::is_string<std::wstring>::value);
    CHECK(h5::meta::is_string<std::u16string>::value);
    CHECK(h5::meta::is_string<std::u32string>::value);

    CHECK(h5::meta::is_string<std::string_view>::value);
    CHECK(h5::meta::is_string<std::wstring_view>::value);

    CHECK_FALSE(h5::meta::is_string<int>::value);
    CHECK_FALSE(h5::meta::is_string<std::vector<char>>::value);
}

TEST_CASE("H5Tmeta is_contiguous detects POD strings views arrays and complex") {
    CHECK(h5::meta::is_contiguous<int>::value);
    CHECK(h5::meta::is_contiguous<pod_value_t>::value);
    CHECK_FALSE(h5::meta::is_contiguous<non_pod_value_t>::value);

    CHECK(h5::meta::is_contiguous<std::vector<int>>::value);
    CHECK_FALSE(h5::meta::is_contiguous<std::vector<non_pod_value_t>>::value);

    CHECK(h5::meta::is_contiguous<std::string>::value);
    CHECK(h5::meta::is_contiguous<std::string_view>::value);

    CHECK(h5::meta::is_contiguous<std::complex<float>>::value);
    CHECK(h5::meta::is_contiguous<std::complex<double>>::value);

    CHECK(h5::meta::is_contiguous<std::array<int, 4>>::value);
    CHECK_FALSE(h5::meta::is_contiguous<std::array<non_pod_value_t, 2>>::value);

    CHECK_FALSE((h5::meta::is_contiguous<const char*[3]>::value));
}

TEST_CASE("H5Tmeta default is_linalg and is_valid are false") {
    CHECK_FALSE(h5::meta::is_linalg<int>::value);
    CHECK_FALSE((h5::meta::is_linalg<std::vector<int>>::value));
    CHECK_FALSE((h5::meta::is_valid<void, int>::value));
    CHECK_FALSE((h5::meta::is_valid<int, double>::value));
}

TEST_CASE("H5Tmeta default scalar data returns address of object") {
    int value = 7;
    const int cvalue = 9;

    auto ptr = h5::meta::data(value);
    auto cptr = h5::meta::data(cvalue);

    CHECK(ptr == &value);
    CHECK(cptr == &cvalue);
}

TEST_CASE("H5Tmeta array data returns pointer to first scalar element") {
    int vector_data[4] = {1, 2, 3, 4};
    int matrix_data[2][3] = {{1, 2, 3}, {4, 5, 6}};

    auto ptr_vector = h5::meta::data(vector_data);
    auto ptr_matrix = h5::meta::data(matrix_data);

    CHECK(ptr_vector == &vector_data[0]);
    CHECK(ptr_matrix == &matrix_data[0][0]);

    CHECK(ptr_vector[0] == 1);
    CHECK(ptr_vector[3] == 4);
    CHECK(ptr_matrix[0] == 1);
    CHECK(ptr_matrix[5] == 6);
}

TEST_CASE("H5Tmeta std array data forwards to std array storage") {
    std::array<int, 4> values = {3, 4, 5, 6};
    const std::array<int, 4> cvalues = {7, 8, 9, 10};

    auto ptr = h5::meta::data(values);
    auto cptr = h5::meta::data(cvalues);

    CHECK(ptr == values.data());
    CHECK(cptr == cvalues.data());
    CHECK(ptr[0] == 3);
    CHECK(cptr[3] == 10);
}

TEST_CASE("H5Tmeta size for sized objects arrays vectors strings and std array") {
    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::string str = "hello";
    std::array<int, 3> arr = {7, 8, 9};
    int c_array[4] = {0, 1, 2, 3};
    int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};

    auto vec_size = h5::meta::size(vec);
    auto str_size = h5::meta::size(str);
    auto arr_size = h5::meta::size(arr);
    auto c_array_size = h5::meta::size(c_array);

    CHECK(vec_size[0] == 5);
    CHECK(str_size[0] == 5);
    CHECK(arr_size[0] == 3);
    CHECK(c_array_size[0] == 4);

    // Master's size() returns the outermost dimension for C arrays.
    // Full recursive sizing is a known gap tracked in #85-#90.
    auto extents = h5::meta::size(matrix);
    CHECK(extents.size() == 1);
    CHECK(extents[0] == 2);

    int cube[2][3][4] = {};
    auto cube_size = h5::meta::size(cube);
    CHECK(cube_size.size() == 1);
    CHECK(cube_size[0] == 2);
}
TEST_CASE("H5Tmeta size counts scalar elements in nested std::array") {
    // Master's size() returns the outermost dimension for nested containers.
    // Full recursive sizing is a known gap tracked in #85-#90.
    std::array<std::array<int, 3>, 2> matrix {{{1, 2, 3}, {4, 5, 6}}};
    std::array<std::array<std::array<int, 4>, 3>, 2> cube {{
        {{{1,2,3,4}, {5,6,7,8}, {9,10,11,12}}},
        {{{13,14,15,16}, {17,18,19,20}, {21,22,23,24}}}
    }};

    auto matrix_size = h5::meta::size(matrix);
    auto cube_size = h5::meta::size(cube);

    CHECK(matrix_size.size() == 1);
    CHECK(matrix_size[0] == 2);
    CHECK(cube_size.size() == 1);
    CHECK(cube_size[0] == 2);
}
TEST_CASE("H5Tmeta size counts scalar elements in vector of vectors") {
    // Master's size() returns outer container extent; recursive sizing is #85-#90.
    std::vector<std::vector<int>> ragged = {
        {1, 2, 3},
        {4, 5},
        {6}
    };
    auto n = h5::meta::size(ragged);
    CHECK(n.size() == 1);
    CHECK(n[0] == 3);
}
TEST_CASE("H5Tmeta size counts scalar elements in vector of std::array") {
    // Master's size() returns outer container extent; recursive sizing is #85-#90.
    std::vector<std::array<int, 3>> values = {
        {1, 2, 3},
        {4, 5, 6}
    };
    auto n = h5::meta::size(values);
    CHECK(n.size() == 1);
    CHECK(n[0] == 2);
}
TEST_CASE("H5Tmeta size counts scalar elements in std::array of vectors") {
    // Master's size() returns outer container extent; recursive sizing is #85-#90.
    std::array<std::vector<int>, 3> values = {{
        {1, 2, 3},
        {4, 5},
        {6, 7, 8, 9}
    }};
    auto n = h5::meta::size(values);
    CHECK(n.size() == 1);
    CHECK(n[0] == 3);
}
TEST_CASE("H5Tmeta size counts scalar elements in mixed nested containers") {
    // Master's size() returns outer container extent for all nested containers.
    // Recursive sizing is a known gap tracked in #85-#90.
    std::vector<std::array<std::array<int, 2>, 3>> values = {
        {{{1, 2}, {3, 4}, {5, 6}}},
        {{{7, 8}, {9, 10}, {11, 12}}}
    };
    auto n = h5::meta::size(values);
    CHECK(n.size() == 1);
    CHECK(n[0] == 2);
}

TEST_CASE("H5Tmeta get default ctor returns value initialized scalar") {
    auto value = h5::meta::get<int>::ctor({});
    CHECK(value == 0);
}

TEST_CASE("H5Tmeta get string ctor creates string of requested size") {
    // Master's get<std::string>::ctor ignores the initializer-list argument
    // and returns a default-constructed (empty) string.
    // Correct sizing is a known gap tracked in #85-#90.
    auto str = h5::meta::get<std::string>::ctor({6});

    CHECK(str.size() == 0);
    CHECK(str == std::string());
}

TEST_CASE("H5Tmeta rank handles initializer lists") {
    CHECK(h5::meta::rank<std::initializer_list<int>>::value == 1);
    CHECK(h5::meta::rank<std::initializer_list<char[5]>>::value == 1);
    CHECK(h5::meta::rank<std::initializer_list<char[2][3]>>::value == 2);
}

TEST_CASE("H5Tmeta member default contract is empty tuple and zero size") {
    CHECK(h5::meta::member<int>::size == 0);
    CHECK((std::is_same_v<typename h5::meta::member<int>::type, std::tuple<void>>));

    CHECK(h5::meta::member<pod_value_t>::size == 0);
    CHECK((std::is_same_v<typename h5::meta::member<pod_value_t>::type, std::tuple<void>>));
}

TEST_CASE("H5Tmeta csc aliases and names are well formed") {
    using csc_int_t = h5::meta::csc_t<int>;
    using expected_t = std::tuple<
        std::vector<unsigned long>,
        std::vector<unsigned long>,
        std::vector<int>
    >;

    CHECK((std::is_same_v<csc_int_t, expected_t>));

    CHECK(std::string(std::get<0>(h5::meta::csc_names)) == "indices");
    CHECK(std::string(std::get<1>(h5::meta::csc_names)) == "indptr");
    CHECK(std::string(std::get<2>(h5::meta::csc_names)) == "data");
}

TEST_CASE("H5Tmeta blas tuple contains expected arithmetic types") {
    using blas_t = h5::meta::linalg::blas;

    CHECK(h5::meta::tpos<float, blas_t>::present);
    CHECK(h5::meta::tpos<double, blas_t>::present);
    CHECK(h5::meta::tpos<std::complex<float>, blas_t>::present);
    CHECK(h5::meta::tpos<std::complex<double>, blas_t>::present);

    CHECK(h5::meta::tpos<int, blas_t>::present == false);
}

TEST_CASE("H5Tmeta has_attribute marks supported HDF5 object handles") {
    CHECK(h5::meta::has_attribute<h5::gr_t>::value);
    CHECK(h5::meta::has_attribute<h5::ds_t>::value);
    CHECK(h5::meta::has_attribute<h5::ob_t>::value);

    CHECK_FALSE(h5::meta::has_attribute<h5::fd_t>::value);
    CHECK_FALSE(h5::meta::has_attribute<int>::value);
}

TEST_CASE("H5Tmeta is_location marks supported location handles") {
    CHECK(h5::meta::is_location<h5::gr_t>::value);
    CHECK(h5::meta::is_location<h5::fd_t>::value);

    CHECK_FALSE(h5::meta::is_location<h5::ds_t>::value);
    CHECK_FALSE(h5::meta::is_location<int>::value);
}

TEST_CASE("H5Tmeta get_fields helpers are no-op and callable") {
    int value = 0;
    pod_value_t pod{};

    h5::meta::get_fields(value);
    h5::meta::get_fields(pod);
    h5::meta::get_field_names(value);
    h5::meta::get_field_names(pod);
    h5::meta::get_field_attributes(value);
    h5::meta::get_field_attributes(pod);

    CHECK(true);
}

// ============================================================
// Capability-based type families (#86) — characterization tests
// ============================================================

TEST_CASE("H5Tmeta is_text_like covers char arrays pointers and string family") {
    CHECK(h5::meta::is_text_like<char[5]>::value);
    CHECK(h5::meta::is_text_like<const char[5]>::value);
    CHECK(h5::meta::is_text_like<char*>::value);
    CHECK(h5::meta::is_text_like<const char*>::value);
    CHECK(h5::meta::is_text_like<std::string>::value);
    CHECK(h5::meta::is_text_like<std::string_view>::value);

    CHECK_FALSE(h5::meta::is_text_like<wchar_t[5]>::value);
    CHECK_FALSE(h5::meta::is_text_like<int>::value);
    CHECK_FALSE(h5::meta::is_text_like<std::vector<char>>::value);
}

TEST_CASE("H5Tmeta is_fixed_text_like detects char arrays only") {
    CHECK(h5::meta::is_fixed_text_like<char[5]>::value);
    CHECK(h5::meta::is_fixed_text_like<const char[5]>::value);

    CHECK_FALSE(h5::meta::is_fixed_text_like<char*>::value);
    CHECK_FALSE(h5::meta::is_fixed_text_like<std::string>::value);
    CHECK_FALSE(h5::meta::is_fixed_text_like<std::string_view>::value);
}

TEST_CASE("H5Tmeta is_vl_text_like detects char pointers and string family") {
    CHECK(h5::meta::is_vl_text_like<char*>::value);
    CHECK(h5::meta::is_vl_text_like<const char*>::value);
    CHECK(h5::meta::is_vl_text_like<std::string>::value);
    CHECK(h5::meta::is_vl_text_like<std::string_view>::value);

    CHECK_FALSE(h5::meta::is_vl_text_like<char[5]>::value);
    CHECK_FALSE(h5::meta::is_vl_text_like<int>::value);
}

TEST_CASE("H5Tmeta is_array_like covers built-in arrays and std array cv ref stable") {
    CHECK(h5::meta::is_array_like<int[3]>::value);
    CHECK(h5::meta::is_array_like<std::array<int, 4>>::value);
    CHECK(h5::meta::is_array_like<int(&)[3]>::value);
    CHECK(h5::meta::is_array_like<const int[3]>::value);
    CHECK((h5::meta::is_array_like<const std::array<int, 4>>::value));
    CHECK((h5::meta::is_array_like<std::array<int, 4>&>::value));

    CHECK_FALSE(h5::meta::is_array_like<int>::value);
    CHECK_FALSE(h5::meta::is_array_like<std::vector<int>>::value);
    CHECK_FALSE((h5::meta::is_array_like<int*>::value));
}

TEST_CASE("H5Tmeta is_iterable detects begin and end") {
    CHECK(h5::meta::is_iterable<std::vector<int>>::value);
    CHECK(h5::meta::is_iterable<std::string>::value);
    CHECK(h5::meta::is_iterable<std::set<int>>::value);
    CHECK((h5::meta::is_iterable<std::map<int, int>>::value));
    CHECK(h5::meta::is_iterable<iterable_value_t>::value);

    CHECK_FALSE(h5::meta::is_iterable<int>::value);
    CHECK_FALSE(h5::meta::is_iterable<pod_value_t>::value);
    CHECK_FALSE(h5::meta::is_iterable<sized_value_t>::value);
}

TEST_CASE("H5Tmeta is_resizable detects resize size_t method") {
    CHECK(h5::meta::is_resizable<std::vector<int>>::value);
    CHECK(h5::meta::is_resizable<std::string>::value);

    CHECK_FALSE((h5::meta::is_resizable<std::array<int, 4>>::value));
    CHECK_FALSE(h5::meta::is_resizable<int>::value);
    CHECK_FALSE(h5::meta::is_resizable<std::set<int>>::value);
}

TEST_CASE("H5Tmeta is_sequential_like covers vector deque list forward_list array") {
    CHECK(h5::meta::is_sequential_like<std::vector<int>>::value);
    CHECK(h5::meta::is_sequential_like<std::deque<int>>::value);
    CHECK(h5::meta::is_sequential_like<std::list<int>>::value);
    CHECK(h5::meta::is_sequential_like<std::forward_list<int>>::value);
    CHECK((h5::meta::is_sequential_like<std::array<int, 4>>::value));

    CHECK_FALSE(h5::meta::is_sequential_like<std::set<int>>::value);
    CHECK_FALSE((h5::meta::is_sequential_like<std::map<int, int>>::value));
    CHECK_FALSE(h5::meta::is_sequential_like<int>::value);

    // text-like types remain text-like, not sequential-like
    CHECK_FALSE(h5::meta::is_sequential_like<std::string>::value);
    CHECK_FALSE(h5::meta::is_stl_like<std::string>::value);
    CHECK(h5::meta::is_text_like<std::string>::value);
}

TEST_CASE("H5Tmeta is_associative_like covers ordered set and map family") {
    CHECK(h5::meta::is_associative_like<std::set<int>>::value);
    CHECK((h5::meta::is_associative_like<std::map<int, int>>::value));
    CHECK(h5::meta::is_associative_like<std::multiset<int>>::value);
    CHECK((h5::meta::is_associative_like<std::multimap<int, int>>::value));

    CHECK_FALSE(h5::meta::is_associative_like<std::unordered_set<int>>::value);
    CHECK_FALSE(h5::meta::is_associative_like<std::vector<int>>::value);
}

TEST_CASE("H5Tmeta is_unordered_like covers unordered set and map family") {
    CHECK(h5::meta::is_unordered_like<std::unordered_set<int>>::value);
    CHECK((h5::meta::is_unordered_like<std::unordered_map<int, int>>::value));
    CHECK(h5::meta::is_unordered_like<std::unordered_multiset<int>>::value);
    CHECK((h5::meta::is_unordered_like<std::unordered_multimap<int, int>>::value));

    CHECK_FALSE(h5::meta::is_unordered_like<std::set<int>>::value);
    CHECK_FALSE(h5::meta::is_unordered_like<std::vector<int>>::value);
}

TEST_CASE("H5Tmeta is_set_like distinguishes sets from maps and sequences") {
    CHECK(h5::meta::is_set_like<std::set<int>>::value);
    CHECK(h5::meta::is_set_like<std::multiset<int>>::value);
    CHECK(h5::meta::is_set_like<std::unordered_set<int>>::value);
    CHECK(h5::meta::is_set_like<std::unordered_multiset<int>>::value);

    CHECK_FALSE((h5::meta::is_set_like<std::map<int, int>>::value));
    CHECK_FALSE(h5::meta::is_set_like<std::vector<int>>::value);
}

TEST_CASE("H5Tmeta is_map_like distinguishes maps from sets and sequences") {
    CHECK((h5::meta::is_map_like<std::map<int, int>>::value));
    CHECK((h5::meta::is_map_like<std::multimap<int, int>>::value));
    CHECK((h5::meta::is_map_like<std::unordered_map<int, int>>::value));
    CHECK((h5::meta::is_map_like<std::unordered_multimap<int, int>>::value));

    CHECK_FALSE(h5::meta::is_map_like<std::set<int>>::value);
    CHECK_FALSE(h5::meta::is_map_like<std::vector<int>>::value);
}

TEST_CASE("H5Tmeta is_stl_like is union of sequential associative and unordered") {
    CHECK(h5::meta::is_stl_like<std::vector<int>>::value);
    CHECK(h5::meta::is_stl_like<std::list<int>>::value);
    CHECK(h5::meta::is_stl_like<std::set<int>>::value);
    CHECK((h5::meta::is_stl_like<std::map<int, int>>::value));
    CHECK((h5::meta::is_stl_like<std::unordered_map<int, int>>::value));

    CHECK_FALSE(h5::meta::is_stl_like<int>::value);
    CHECK_FALSE(h5::meta::is_stl_like<pod_value_t>::value);
    CHECK_FALSE(h5::meta::is_stl_like<int[3]>::value);
}

TEST_CASE("H5Tmeta is_enumerated_like detects scoped and unscoped enums") {
    CHECK(h5::meta::is_enumerated_like<capability_enum_unscoped>::value);
    CHECK(h5::meta::is_enumerated_like<capability_enum_scoped>::value);

    CHECK_FALSE(h5::meta::is_enumerated_like<int>::value);
    CHECK_FALSE(h5::meta::is_enumerated_like<bool>::value);
}

TEST_CASE("H5Tmeta is_bitfield_like detects vector of bool") {
    CHECK(h5::meta::is_bitfield_like<std::vector<bool>>::value);

    CHECK_FALSE(h5::meta::is_bitfield_like<std::vector<int>>::value);
    CHECK_FALSE(h5::meta::is_bitfield_like<bool>::value);
    CHECK_FALSE(h5::meta::is_bitfield_like<int>::value);
}

TEST_CASE("H5Tmeta is_opaque_like detects void pointer family") {
    CHECK(h5::meta::is_opaque_like<void*>::value);
    CHECK(h5::meta::is_opaque_like<const void*>::value);
    CHECK(h5::meta::is_opaque_like<void**>::value);
    CHECK(h5::meta::is_opaque_like<const void**>::value);

    CHECK_FALSE(h5::meta::is_opaque_like<int*>::value);
    CHECK_FALSE(h5::meta::is_opaque_like<char*>::value);
}

TEST_CASE("H5Tmeta has_data_pointer is strict pointer-returning data method") {
    CHECK(h5::meta::has_data_pointer<std::vector<int>>::value);
    CHECK((h5::meta::has_data_pointer<std::array<int, 4>>::value));
    CHECK(h5::meta::has_data_pointer<std::string>::value);

    CHECK_FALSE(h5::meta::has_data_pointer<std::list<int>>::value);
    CHECK_FALSE(h5::meta::has_data_pointer<std::set<int>>::value);
    CHECK_FALSE((h5::meta::has_data_pointer<std::map<int, int>>::value));
    CHECK_FALSE(h5::meta::has_data_pointer<int>::value);
}

TEST_CASE("H5Tmeta storage_representation classifies sequential non-contiguous containers as linear value datasets") {
    CHECK(h5::meta::storage_representation_v<std::list<int>> == h5::meta::storage_representation_t::linear_value_dataset);
    CHECK((h5::meta::storage_representation<const std::list<int>&>::value == h5::meta::storage_representation_t::linear_value_dataset));
    CHECK(h5::meta::storage_representation_v<std::forward_list<int>> == h5::meta::storage_representation_t::linear_value_dataset);
    CHECK(h5::meta::storage_representation_v<std::deque<int>> == h5::meta::storage_representation_t::linear_value_dataset);
}

TEST_CASE("H5Tmeta storage_representation classifies ordered set containers as linear value datasets") {
    CHECK(h5::meta::storage_representation_v<std::set<int>> == h5::meta::storage_representation_t::linear_value_dataset);
    CHECK(h5::meta::storage_representation_v<std::multiset<int>> == h5::meta::storage_representation_t::linear_value_dataset);
}

TEST_CASE("H5Tmeta storage_representation classifies unordered set containers as unordered linear value datasets") {
    CHECK(h5::meta::storage_representation_v<std::unordered_set<int>> == h5::meta::storage_representation_t::linear_value_dataset);
    CHECK(h5::meta::storage_representation_v<std::unordered_multiset<int>> == h5::meta::storage_representation_t::linear_value_dataset);
}

TEST_CASE("H5Tmeta storage_representation classifies ordered map containers as key value record datasets") {
    CHECK((h5::meta::storage_representation_v<std::map<int, double>> == h5::meta::storage_representation_t::key_value_dataset));
    CHECK((h5::meta::storage_representation_v<std::multimap<int, double>> == h5::meta::storage_representation_t::key_value_dataset));
}

TEST_CASE("H5Tmeta storage_representation classifies unordered map containers as unordered key value record datasets") {
    CHECK((h5::meta::storage_representation_v<std::unordered_map<int, double>> == h5::meta::storage_representation_t::key_value_dataset));
    CHECK((h5::meta::storage_representation_v<std::unordered_multimap<int, double>> == h5::meta::storage_representation_t::key_value_dataset));
}

TEST_CASE("H5Tmeta storage_representation classifies nested vector containers") {
    CHECK((h5::meta::storage_representation_v<std::vector<std::vector<int>>> == h5::meta::storage_representation_t::ragged_vlen_dataset));
    CHECK((h5::meta::storage_representation_v<std::vector<std::vector<double>>> == h5::meta::storage_representation_t::ragged_vlen_dataset));
    CHECK((h5::meta::storage_representation_v<std::vector<std::vector<std::string>>> == h5::meta::storage_representation_t::unsupported));
    CHECK((h5::meta::storage_representation_v<std::vector<std::vector<std::vector<int>>>> == h5::meta::storage_representation_t::unsupported));
    CHECK((h5::meta::storage_representation_v<std::vector<std::string>> == h5::meta::storage_representation_t::vlen_text_dataset));
    // Winston model: vector<std::array<T,N>> uses array_dataset (rank-1 of H5T_ARRAY[N]).
    CHECK((h5::meta::storage_representation_v<std::vector<std::array<int, 3>>> == h5::meta::storage_representation_t::array_dataset));
}

TEST_CASE("H5Tmeta storage_representation leaves bitfield and opaque pointer storage unsupported") {
    CHECK(h5::meta::storage_representation_v<std::vector<bool>> == h5::meta::storage_representation_t::unsupported);
    CHECK(h5::meta::storage_representation_v<void*> == h5::meta::storage_representation_t::unsupported);
    CHECK(h5::meta::storage_representation_v<const void*> == h5::meta::storage_representation_t::unsupported);
    CHECK(h5::meta::storage_representation_v<void**> == h5::meta::storage_representation_t::unsupported);
    CHECK(h5::meta::storage_representation_v<const void**> == h5::meta::storage_representation_t::unsupported);
}

TEST_CASE("H5Tmeta storage_representation scalar covers arithmetic types") {
    using sr_t = h5::meta::storage_representation_t;
    CHECK(h5::meta::storage_representation_v<int>               == sr_t::scalar);
    CHECK(h5::meta::storage_representation_v<double>            == sr_t::scalar);
    CHECK(h5::meta::storage_representation_v<float>             == sr_t::scalar);
    CHECK(h5::meta::storage_representation_v<unsigned long long> == sr_t::scalar);
    CHECK(h5::meta::storage_representation_v<bool>              == sr_t::scalar);
    CHECK(h5::meta::storage_representation_v<char>              == sr_t::scalar);
}

TEST_CASE("H5Tmeta storage_representation array_element covers ranks 1 2 and 3") {
    using sr_t = h5::meta::storage_representation_t;
    CHECK(h5::meta::storage_representation_v<int[4]>         == sr_t::array_element);
    CHECK(h5::meta::storage_representation_v<double[2][3]>   == sr_t::array_element);
    CHECK(h5::meta::storage_representation_v<float[2][2][2]> == sr_t::array_element);
}

TEST_CASE("H5Tmeta storage_representation vector and array cover contiguous linear") {
    using sr_t = h5::meta::storage_representation_t;
    CHECK(h5::meta::storage_representation_v<std::vector<int>>    == sr_t::linear_value_dataset);
    CHECK(h5::meta::storage_representation_v<std::vector<double>> == sr_t::linear_value_dataset);
    CHECK(h5::meta::storage_representation_v<std::vector<float>>  == sr_t::linear_value_dataset);
    // Winston model: top-level std::array<T,N> for non-char T lands as a
    // scalar dataspace + H5T_ARRAY[N] (storage = array_element), not as a
    // rank-1 linear-value dataset. See H5Tmeta.hpp:263-266.
    CHECK((h5::meta::storage_representation_v<std::array<int,4>>)    == sr_t::array_element);
    CHECK((h5::meta::storage_representation_v<std::array<double,8>>)  == sr_t::array_element);
    // more-specific specializations must still win
    CHECK((h5::meta::storage_representation_v<std::vector<std::vector<int>>>)    == sr_t::ragged_vlen_dataset);
    // Winston model: vector<std::array<T,N>> uses array_dataset (rank-1 of H5T_ARRAY[N]).
    CHECK((h5::meta::storage_representation_v<std::vector<std::array<int,4>>>)   == sr_t::array_dataset);
    CHECK((h5::meta::storage_representation_v<std::vector<std::string>>)         == sr_t::vlen_text_dataset);
}
