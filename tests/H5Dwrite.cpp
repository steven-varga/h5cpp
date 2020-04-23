/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define H5CPP_WRITE_DISPATCH_TEST_(T, msg){ \
    std::cout << msg; \
} \


#include "utils/abstract.hpp"

//#include "../h5cpp/H5Uall.hpp"
#include "../h5cpp/H5Dcreate.hpp"
#include "../h5cpp/H5Dopen.hpp"
#include "../h5cpp/H5Dwrite.hpp"

namespace ns = h5::test;
template <typename T> struct H5D : public TestWithOpenHDF5<T> {};
struct H5Dstring : public TestWithOpenHDF5<char*> {};
typedef ::testing::Types<
char, int, float*, h5::test::pod_t* //, std::string
//unsigned char, unsigned short, unsigned int, unsigned long long int,
//		char, short, int, long long int, float, double, h5::test::pod_t, std::string
> element_t;
TYPED_TEST_SUITE(H5D, element_t, ns::element_names_t);

H5D_layout_t get_layout(const h5::ds_t& ds ){
    h5::dcpl_t dcpl{H5Dget_create_plist(ds)};
    return H5Pget_layout(dcpl);
}
/*
TEST_F(H5Dstring, literal_wo){
    //h5::ds_t ds = h5::create<char[10]>(this->fd, this->name);
}
TEST_F(H5Dstring, literal_cw){
    h5::write(this->fd, this->name, "1234567890 abcdefg jklm   ");
}

TEST_F(H5Dstring, from_variable_compact){
    const char var[] = "hello world!";
    h5::write(this->fd, this->name, var);
}
TEST_F(H5Dstring, from_variable_contigious){
    constexpr size_t length = H5CPP_COMPACT_PAYLOAD_MAX_SIZE+5;
    char var[length]={0};
    std::fill(var, var + length-1, 'a');
    //h5::write(this->fd, this->name, var);
}
*/

TEST_F(H5Dstring, compact_1D){
    //h5::write(this->fd, this->name,"this is a character literal...");
}
TEST_F(H5Dstring, compact_2D){
    const char var[][10] = {"first","second","third","...","last"};
    h5::write(this->fd, this->name, var);
}
TEST_F(H5Dstring, compact_3D){
    const char var[][12][10] = {
        {"00","second","third","...","last"},
        {"01","second","third","...","last"},
        {"02","second","third","...","last"}
    };
    h5::write(this->fd, this->name, var);
}
/*
TEST(H5D, string_object){
    h5::ds_t ds;
    std::string ref("String Object");
    h5::write(ds, std::string("hello"));
}
TEST(H5D, initializer_list_string_1){
    h5::ds_t ds;
    std::initializer_list<std::string> ref({"1","2","3","4","...","N"});
    h5::write(ds, ref);
}
TEST(H5D, initializer_list_string_2){
    h5::ds_t ds;
    h5::write(ds, {"1","2","3","4","...","N"});
}
TEST(H5D, initializer_list_int){
    h5::ds_t ds;
    h5::write(ds, {1,2,3,4,0,10});
}
TEST(H5D, initializer_list_float){
    h5::ds_t ds;
    h5::write(ds, {1.0,2.0,3.0,4.0,0.0,10.0});
}

TYPED_TEST(H5D, array_1d) {
    TypeParam data[4];
    h5::ds_t ds;
    h5::write(ds, data);
}
TYPED_TEST(H5D, array_2d) {
    TypeParam data[4][3];
    h5::ds_t ds;
    h5::write(ds, data);
}
TYPED_TEST(H5D, array_3d) {
    TypeParam data[4][3][2];
    h5::ds_t ds;
    h5::write(ds, data);
}
TYPED_TEST(H5D, ptr_) {
    TypeParam* ptr;
    h5::ds_t ds;
    h5::write(ds, ptr);
}
TYPED_TEST(H5D, ptr__) {
    TypeParam** ptr;
    h5::ds_t ds;
    h5::write(ds, ptr);
}

TYPED_TEST(H5D, T) {
    TypeParam data;
    h5::ds_t ds;
    h5::write(ds, data);
}
*/

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/

