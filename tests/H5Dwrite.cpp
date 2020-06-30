/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#define H5CPP_WRITE_DISPATCH_TEST_(T, msg){ \
    std::cout << "macro: " << msg << "\n"; \
}; \


#include "utils/abstract.hpp"

//#include "../h5cpp/H5Uall.hpp"
#include "../h5cpp/H5Dcreate.hpp"
#include "../h5cpp/H5Dopen.hpp"
#include "../h5cpp/H5Dwrite.hpp"

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
/*
TEST_F(H5Dstring, partial_io){
    h5::write(this->fd, this->name, h5::count{1,1}, std::string("std::string"), h5::current_dims{3,5}, h5::offset{2,2});
}

TEST_F(H5Dstring, initalizer_list){
    //h5::ds_t ds = h5::create<std::string>(this->fd, this->name, h5::current_dims{7}, h5::max_dims{20});
    //h5::write(ds, {"1","2","3","4","...","N-1","N"});
    h5::write<char[20]>(this->fd, this->name, {"1","2","3","4","...","N-1","N"});
}
*/
/*
TEST_F(H5Dstring, fixed_abc){
    const char* var1[] = {"A","B","C","...","Z"};
    //const char var2[][5] = {"A","B","C","...","Z"};
    //std::string var3 = "hello";
    //const char var4[] = "world";
    //auto ds = h5::create<char[5][10]>(this->fd, this->name);
    //auto ds = h5::create<std::string>(this->fd, this->name, h5::current_dims{10});
    h5::write(this->fd, this->name + "-array",  var1);
//    h5::write<std::string>(this->fd, this->name + "-hyperslab", var, h5::offset{3}, h5::count{5},  h5::current_dims{10}, h5::max_dims{15});
    //    h5::write(ds, var, h5::offset{4}, h5::count{5});
}
*/
TEST_F(H5Dstring,  std_array) {
    std::array<std::string,3> data({"A","B","C"});
    h5::write(this->fd,this->name, data);
}
TEST_F(H5Dstring, from_variable_chunked_vector){
    const char* var[] = {"A","B","C","...","Z"};
    auto ds = h5::create<std::string>(this->fd, this->name, h5::current_dims{10}, h5::max_dims{15});
    h5::write(ds, var, h5::offset{4}, h5::count{5});
}

TEST_F(H5Dstring, from_var_to_vl_string_vector){
    const char* var[] = {"A","B","C","...","Z"};
    auto ds = h5::create<std::string[5]>(this->fd, this->name);
    h5::write(ds, var);
}

TEST_F(H5Dstring, array_vl_string){
    h5::ds_t ds = h5::create<std::string[6]>(this->fd, this->name);
    h5::write(ds, {"1","2","3","4","...","N"});
}

TEST_F(H5Dstring, array_vl_char_pointer){
    h5::ds_t ds = h5::create<char const*[6]>(this->fd, this->name);
    h5::write(ds, {"1","2","3","4","...","N"});
}

/*
TEST_F(H5Dstring, from_var_fixed_string_vector){
    const char var[][10] = {"A","B","C","...","Z"};
 //   auto ds = h5::create<char[5][10]>(this->fd, this->name);
 //   h5::write(ds, var);
    h5::write(this->fd, this->name + "-direct", var);
}
*/
TEST_F(H5Dstring, std_array_of_std_string){
    h5::write(this->fd, this->name, std::array<std::string,3>{"A","B","..."});
}

/*
TEST_F(H5Dstring,  std_vector_of_std_string) {
    std::vector<std::string> data = {"A","B","...","Z"};
    h5::write(this->fd,this->name, data, h5::max_dims{100});
}
TEST_F(H5Dvlstring, initializer_list){ // H5_ARRAY{VL_STRING}
    h5::write(this->fd, this->name, {"A","B","C","...","Z"});
}
*/

TEST_F(H5Dvlstring,  partial_io_vector) {
    std::vector<std::string> data({"1","2","3","4","5","F","G","H","J"});
    h5::write(this->fd, this->name, data, h5::current_dims{30}, h5::max_dims{40}, h5::offset{5});
}


TEST_F(H5Dvlstring,  partial_io_array) {
    std::array<std::string,9> data({"1","2","3","4","5","F","G","H","J"});
    // some cases you have to indicate type
    h5::write<std::string>(this->fd, this->name, data, h5::current_dims{30}, h5::max_dims{40}, h5::offset{5});
}
TEST_F(H5Dvlstring,  std_vector) {
    std::vector<std::string> data({"1","2","3","4","5","F","G","H","J"});
    h5::write(this->fd, this->name, data);
}

TEST_F(H5Dvlstring,  crw_std_string) {
    h5::write(this->fd,this->name, std::string("hello there"));
}
TEST_F(H5Dvlstring,  array) { // file_type
    std::array<std::string,9> data({"1","2","3","4","5","F","G","H","J"});
    h5::write<std::string[9]>(this->fd, this->name, data);
}



TEST_F(H5Dstring, cr_std_string){
    h5::ds_t ds = h5::create<std::string>(this->fd, this->name);
    h5::write(ds, std::string("hello there"));
}


// matrix[3,4] --> matrix[3][4]
TEST_F(H5Dstring, from_variable_chunked_scalar){
    const char var[] = "hello world!";
    auto ds = h5::create<char[20]>(this->fd, this->name, h5::current_dims{10}, h5::max_dims{15});
    h5::write(ds, var, h5::offset{4}, h5::count{1});
}

TEST_F(H5Dstring, array_1D){
    h5::write(this->fd, this->name,"this is a character literal...");
}

TEST_F(H5Dstring, array_2D){
    const char var[][10] = {"first","second","third","...","last"};
    h5::write(this->fd, this->name, var);
}

TEST_F(H5Dstring, array_3D){
    const char var[][12][10] = {
        {"00","second","third","...","last"},
        {"01","second","third","...","last"},
        {"02","second","third","...","last"}
    };
    h5::write(this->fd, this->name, var);
}

TEST_F(H5Dstring, from_variable_contigious){
    constexpr size_t length = H5CPP_COMPACT_PAYLOAD_MAX_SIZE+5;
    char var[length]={0};
    std::fill(var, var + length-1, 'a');
    h5::write(this->fd, this->name, var);
}


TEST_F(H5Dint,  std_vector) {
    std::vector<int> data({1,2,3});
    //h5::create<short>(this->fd,this->name, h5::current_dims{30}, h5::max_dims{200}, h5::chunk{5} );
    h5::write(this->fd,this->name, data, h5::offset{10}, h5::current_dims{20});
}
TEST_F(H5Dint, initializer_list){
    h5::write(this->fd,this->name, {1,2,3,4,0,10});
}

TEST_F(H5Dlong, initializer_list){
    h5::write(this->fd,this->name, {1L,2L,3L,4L,0L,10L});
}
TEST_F(H5Dlonglong, initializer_list){
    h5::write(this->fd,this->name, {1LL,2LL,3LL,4LL,0LL,10LL});
}

TEST_F(H5Dfloat, initializer_list){
    h5::write(this->fd, this->name, {1.0f,2.0f,3.0f,4.0f,0.0f,10.0f});
}
TEST_F(H5Ddouble, initializer_list){
    h5::write(this->fd, this->name, {1.0,2.0,3.0,4.0,0.0,10.0});
}
TEST_F(H5Dlongdouble, initializer_list){
    h5::write(this->fd, this->name, {1.0L,2.0L,3.0L,4.0L,0.0L,10.0L});
}

TEST_F(H5Dsignedchar,  array){
    signed char data[] = {1,2,3,4,5,10};
    h5::write(this->fd,this->name, data);
}

TEST_F(H5Dunsignedchar,  array){
    unsigned char data[] = {1,2,3,4,5,10};
    h5::write(this->fd,this->name, data);
}
TEST_F(H5Dbyte,  array){
    std::byte data[5]; // = {1,2,3,4,5,10};
    h5::write(this->fd,this->name, data);
}

TEST_F(H5Dint,  array){
    unsigned char data[] = {1,2,3,4,5,10};
    h5::write(this->fd,this->name, data);
}


TEST_F(H5Dfloat,  array){
    float data[] = {1.0f,2.0f,3.0f,4.0f,5.0f,10.0f};
    h5::write(this->fd,this->name, data);
}
TEST_F(H5Dlongdouble,  array){
    long double data[] = {1.0L,2.0L,3.0L,4.0L,5.0L,10.0L};
    h5::write(this->fd,this->name, data);
}

TEST_F(H5Dint,  std_array) {
    std::array<int,6> data({1,2,3,4,5,10});
    h5::write(this->fd,this->name, data);
}

TEST_F(H5Dpod,  std_array_of_std_array) {
    std::array<std::array<h5::test::pod_t,3>,2> data = {1,2,3,4,5,6,7,8,9, 10,11,12,13,14,15,16,17,18};
    h5::write(this->fd,this->name, data);
}

TEST_F(H5Dpod,  std_array_of_array) {
    std::array<h5::test::pod_t[2],3> data = {1,2,3,4,5,6,7,8,9, 10,11,12,13,14,15,16,17,18};
    h5::write(this->fd,this->name, data);
}

TEST_F(H5Dpod,  array_of_std_array) {
    std::array<h5::test::pod_t,2> data[3] = {1,2,3,4,5,6,7,8,9, 10,11,12,13,14,15,16,17,18};
    h5::write(this->fd,this->name, data);
}

TYPED_TEST(H5D, vector_compact_layout){
    constexpr size_t length = (H5CPP_COMPACT_PAYLOAD_MAX_SIZE / sizeof(TypeParam)) - 1;
    std::vector<TypeParam> data(length);
//    h5::ds_t ds = h5::write(this->fd, this->name, data);
//    ASSERT_EQ(get_layout(ds), H5D_COMPACT);
}

TYPED_TEST(H5D, vector_contigous_layout){
    constexpr size_t length = (H5CPP_COMPACT_PAYLOAD_MAX_SIZE / sizeof(TypeParam)) + 1;
    std::vector<TypeParam> data(length);
  //  h5::ds_t ds = h5::write(this->fd, this->name, data);
  //  ASSERT_EQ(get_layout(ds), H5D_CONTIGUOUS);
}

TYPED_TEST(H5D, vector_chunked_layout_size){
    constexpr size_t length = (H5CPP_COMPACT_PAYLOAD_MAX_SIZE / sizeof(TypeParam)) + 1;
    std::vector<TypeParam> data(length);
//    h5::ds_t ds = h5::write(this->fd, this->name, data, h5::max_dims{1000000});
//    ASSERT_EQ(get_layout(ds), H5D_CHUNKED);
}

TYPED_TEST(H5D, vector_chunked_layout_filters_on){
    std::vector<TypeParam> data(128);
    auto dcpl = h5::default_dcpl;
    h5::ds_t ds = h5::write(this->fd, this->name, data, h5::chunk{10} | h5::gzip{9});
    ASSERT_EQ(get_layout(ds), H5D_CHUNKED);
}

TYPED_TEST(H5D, vector_chunked_layout_chunk_on){
    std::vector<TypeParam> data(128);
    auto dcpl = h5::default_dcpl;
    h5::ds_t ds = h5::write(this->fd, this->name, data, h5::chunk{10});
    ASSERT_EQ(get_layout(ds), H5D_CHUNKED);
}
TEST_F(H5Dshort,  std_vector) {
    std::vector<short> data = {1,2,3,4,10};
    auto ds = h5::write(this->fd,this->name, data, h5::max_dims{100});
}

TEST_F(H5Dchar,  std_vector) {
    std::vector<char> data = {1,2,3,4,10}, out(data.size());
    h5::ds_t ds = h5::write(this->fd,this->name, data, h5::max_dims{100});
    h5::dt_t<char> mem_type = h5::create<char>();
    H5Dread(ds, mem_type, H5S_ALL, H5S_ALL, h5::default_dxpl, out.data());
    for(int i=0; i<data.size(); i++) ASSERT_EQ(data[i], out[i]);
}

/*----------- BEGIN TEST RUNNER ---------------*/
H5CPP_BASIC_RUNNER( int argc, char**  argv );
/*----------------- END -----------------------*/
