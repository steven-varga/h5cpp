/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


#include <armadillo>

#define H5CPP_USE_ARMADILLO

#include "utils/abstract.hpp"
#include "../h5cpp/H5Dcreate.hpp"
#include "../h5cpp/H5Dopen.hpp"
#include "../h5cpp/H5Acreate.hpp"
#include "../h5cpp/H5Aopen.hpp"
#include "../h5cpp/H5Awrite.hpp"
#include "../h5cpp/H5Aread.hpp"
#include "../h5cpp/H5Dwrite.hpp"
#include "../h5cpp/H5Dread.hpp"
#include "../h5cpp/H5Marma.hpp"


namespace ns = h5::test;
template <typename T> struct H5D : public TestWithOpenHDF5<T> {};
struct H5Dstring : public TestWithOpenHDF5<char*> {};
struct H5Dvlstring : public TestWithOpenHDF5<std::string> {};
struct H5Dchar : public TestWithOpenHDF5<char> {};                  // H5T_STRING
struct H5Dunsignedchar : public TestWithOpenHDF5<unsigned char> {}; // H5T_UCHAR numerical  
struct H5Dsignedchar : public TestWithOpenHDF5<signed char> {};     // H5T_CHAR numerical
struct H5Dbyte : public TestWithOpenHDF5<std::byte> {};             // H5T_BYTE numerical
struct H5Dshort : public TestWithOpenHDF5<short> {};
struct H5Dint : public TestWithOpenHDF5<int> {};
struct H5Dlong : public TestWithOpenHDF5<long> {};
struct H5Dlonglong: public TestWithOpenHDF5<long long> {};
struct H5Dfloat : public TestWithOpenHDF5<float> {};
struct H5Ddouble : public TestWithOpenHDF5<double> {};
struct H5Dlongdouble : public TestWithOpenHDF5<long double> {};
struct H5Dpod : public TestWithOpenHDF5<h5::test::pod_t> {};
typedef ::testing::Types<
char, int, float, h5::test::pod_t //, std::string
//unsigned char, unsigned short, unsigned int, unsigned long long int,
//		char, short, int, long long int, float, double, h5::test::pod_t, std::string
> element_t;
TYPED_TEST_SUITE(H5D, element_t, ns::element_names_t);

H5D_layout_t get_layout(const h5::ds_t& ds ){
    h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
    return H5Pget_layout(dcpl);
}
TEST_F(H5Dshort, from_variable_chunked_vector){
    short var[] = {1,2,3,4,5};
    std::vector<short> out(5);
    h5::write(this->fd, this->name, var);
    h5::read<short>(this->fd, this->name, out.data(), h5::count{5});
}

/*
TEST_F(H5Dstring,  std_vector) {
    std::array<std::string,9> data({"1","2","3","4","5","F","G","H","J"});

    h5::ds_t ds = h5::write<std::string>(this->fd,this->name, data, h5::current_dims{30}, h5::max_dims{40}, h5::offset{5});
    h5::sp_t sp{H5Dget_space(ds)};
    char* ptr[30]; hsize_t size;
    h5::dt_t<char**> mem_type = h5::create<char**>();
    H5Dvlen_get_buf_size(ds, mem_type, sp, &size );
    h5::dxpl_t dxpl  = h5::pooling_manager{8096};

    std::cout << H5Dread(ds, mem_type, H5S_ALL, H5S_ALL, dxpl, ptr);
    H5Dvlen_reclaim (mem_type, sp, dxpl, ptr);
}
*/

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

