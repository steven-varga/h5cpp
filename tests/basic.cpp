
#include <armadillo>
#include <h5cpp>
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <iterator>
#include <vector>
#include <random>
#include <algorithm>



template<typename T> struct TypeParseTraits;

#define H5CPP_DEF_TYPE_NAME( X ) template <> struct TypeParseTraits<X> {   \
					static const char* name; 								\
} ; 																		\
const char* TypeParseTraits<X>::name = #X 	 								\

//template <> std::string get_type_name<NAME>(){ return std::string( NAME ); }

H5CPP_DEF_TYPE_NAME(unsigned char); H5CPP_DEF_TYPE_NAME(char);
H5CPP_DEF_TYPE_NAME(unsigned short); H5CPP_DEF_TYPE_NAME(short);

H5CPP_DEF_TYPE_NAME(unsigned int); H5CPP_DEF_TYPE_NAME(int);
H5CPP_DEF_TYPE_NAME(unsigned long long int); H5CPP_DEF_TYPE_NAME(long long int);
H5CPP_DEF_TYPE_NAME(float); H5CPP_DEF_TYPE_NAME(double);
H5CPP_DEF_TYPE_NAME(long double); H5CPP_DEF_TYPE_NAME(bool);
H5CPP_DEF_TYPE_NAME(std::string);

#undef H5CPP_DEF_TYPE_NAME

template <typename T> std::vector<T> get_test_data( size_t n ){
	std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<> dist(0,n);

	std::vector<T> data;
	data.reserve(n);
	std::generate_n(std::back_inserter(data), n, [&]() {
							return dist(rng);
						});
	return data;
}

template <> std::vector<std::string> get_test_data( size_t n ){

	std::vector<std::string> data;
	data.reserve(n);

	static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
        							"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
    std::uniform_int_distribution<> string_length(5,30);

	std::generate_n(std::back_inserter(data), data.capacity(),   [&] {
			std::string str;
			size_t N = string_length(rng);
            str.reserve(N);
            std::generate_n(std::back_inserter(str), N, [&]() {
							return alphabet[dist(rng)];
						});
              return str;
			  });
	return data;
}



/** test harness to open/close HDF5 file 
 *
 */ 
template <typename T> class AbstractTest
					: public ::testing::Test {
public:
	void SetUp() {
		dir = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		type = std::string( TypeParseTraits<T>::name);
		name = dir + "/" + type;
		this->fd = h5::open("test.h5", H5F_ACC_RDWR );
	}
	void TearDown() {
		H5Fclose(fd);
	}
	std::string type;
	std::string name;
	std::string dir;
	hid_t fd; //< file descriptor
};
template <typename T> class ArmadilloTest : public AbstractTest<T>{};
template <typename T> class STLTest : public AbstractTest<T>{};
template <typename T> class STLTest_nostring : public AbstractTest<T>{};

typedef ::testing::Types<
	unsigned char, unsigned short, unsigned int, unsigned long long int,
   	char,short, int, long long int,
	float, double, std::string > StlTypes;

typedef ::testing::Types<
	unsigned char, unsigned short, unsigned int, unsigned long long int,
   	char, short, int, long long int,
	float, double > NoStringTypes;

// instantiate for listed types
TYPED_TEST_CASE(STLTest, StlTypes);
TYPED_TEST_CASE(STLTest_nostring, NoStringTypes);
TYPED_TEST_CASE(ArmadilloTest, NoStringTypes);

TYPED_TEST(STLTest, CreateIntegral) {

	// PLAIN
	if( h5::create<TypeParam>(this->fd,this->name + " 1d",{3}) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d",{2,3} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d",{1,2,3}) < 0 ) ADD_FAILURE();
	// CHUNKED
	if( h5::create<TypeParam>(this->fd,this->name + " 1d chunk",{3000},{100} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d chunk",{2000,3},{100,1} ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d chunk",{1000,2,3},{100,1,1} ) < 0 ) ADD_FAILURE();
	// CHUNKED COMPRESSION: GZIP 9
	if( h5::create<TypeParam>(this->fd,this->name + " 1d chunk gzip",{3000},{100},9 ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 2d chunk gzip",{2000,3},{100,1},9 ) < 0 ) ADD_FAILURE();
	if( h5::create<TypeParam>(this->fd,this->name + " 3d chunk gzip",{1000,2,3},{100,1,1},9 ) < 0 ) ADD_FAILURE();
}
/** 
 * @brief WRITE test of armadillo COLUMN | ROW | MAT | CUBE types  
 */ 
TYPED_TEST(ArmadilloTest, WriteArmadillo) {

	arma::Col<TypeParam>  C(10);
	arma::Row<TypeParam>  R(10);
	arma::Mat<TypeParam>  M(10,50);
	arma::Cube<TypeParam> Q(10,50,30);

	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);
	h5::write(this->fd,this->name+".cube", Q);
}

/** 
 * @brief READ test of armadillo COLUMN | ROW | MAT | CUBE types  
 */ 
TYPED_TEST(ArmadilloTest, ReadArmadillo) {

	arma::Col<TypeParam>  C(1000);
	arma::Row<TypeParam>  R(1000);
	arma::Mat<TypeParam>  M(1000,50);
	arma::Cube<TypeParam> Q(1000,50,30);

	// PLAIN
	h5::write(this->fd,this->name+".col", C);
	h5::write(this->fd,this->name+".row", R);
	h5::write(this->fd,this->name+".mat", M);
	h5::write(this->fd,this->name+".cube", Q);
	{ // READ ENTIRE DATASET
	auto c = h5::read<arma::Col<TypeParam>>(this->fd,this->name+".col");
	auto r = h5::read<arma::Row<TypeParam>>(this->fd,this->name+".row");
	auto m = h5::read<arma::Mat<TypeParam>>(this->fd,this->name+".mat");
	auto q = h5::read<arma::Cube<TypeParam>>(this->fd,this->name+".cube");
	}
	{ //READ PARTIAL DATASET
	auto c = h5::read<arma::Col<TypeParam>>(this->fd,this->name+".col",{100},{10} );
	auto r = h5::read<arma::Row<TypeParam>>(this->fd,this->name+".row",{0},{10});
	auto m = h5::read<arma::Mat<TypeParam>>(this->fd,this->name+".mat",{0,0},{10,10});
	auto q = h5::read<arma::Cube<TypeParam>>(this->fd,this->name+".cube",{0,0,0},{10,10,10});
	}

}


TYPED_TEST(STLTest, WriteSTL) {

	std::vector<TypeParam> vec = get_test_data<TypeParam>(100);
	// PLAIN
	h5::write(this->fd,this->name, vec);
}

TYPED_TEST(STLTest, ReadSTL) {

	std::vector<TypeParam> vec = get_test_data<TypeParam>(100);
	h5::write(this->fd,this->name, vec);
	{  // READ
		std::vector<TypeParam> data =  h5::read<std::vector<TypeParam>>(this->fd,this->name );
	}
	{ // PARTIAL READ 
		std::vector<TypeParam> data =  h5::read<std::vector<TypeParam>>(this->fd,this->name,{50},{10} );
	}
}

TYPED_TEST(STLTest_nostring, MultiDim_no_chunk) {
	hid_t ds = h5::create<TypeParam>(this->fd, this->name, {20,200,40}  );
	std::vector<TypeParam> vec = get_test_data<TypeParam>(20);

	// partial write
	h5::write(ds, vec.data(), {0,0,0},{1,20,1} );

	H5Dclose(ds);
}
TYPED_TEST(STLTest_nostring, MultiDim_yes_chunk_gzip) {
	hid_t ds = h5::create<TypeParam>(this->fd, this->name, {20,200,40},{20,10,10}, 9 );
	std::vector<TypeParam> vec = get_test_data<TypeParam>(20);

	// partial write
	h5::write(ds, vec.data(), {0,0,0},{1,20,1} );

	H5Dclose(ds);
}
/**
 * @brief clears error stack and sets FAILURE for google test framework
 *
 */
herr_t gtest_hdf5_error_handler (int a, void *unused) {
	hid_t es = H5Eget_current_stack();
	H5Eclear( es );
	ADD_FAILURE();
}


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  	// make sure dataset exists, and is zapped
  	hid_t fd = h5::create("test.h5");   h5::close(fd);
	//install error handler that captures any h5 error
	hid_t es = H5Eget_current_stack();
	H5Eset_auto(H5E_DEFAULT,gtest_hdf5_error_handler, nullptr );

  	// will create/read/write/append to datasets in fd
	return RUN_ALL_TESTS();
}
