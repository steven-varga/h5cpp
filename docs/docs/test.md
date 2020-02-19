# Cartesian Test
Google Test framework v1.7.0 provides limited support for Cartesian products of test values with `Combine` upto 10 independent cases (see: `gtest-param-test.h`). This restriction is the result of macro based approach, and is lifted by providing a template meta-programming based C++17 compliant implementation.

In `h5cpp-test.hpp` the definition  `#define H5CPP_ALL_TYPES char, short, ... , double` controls what types test cases are iterated through, there is a different type definition for containers, and linear algebra objects. 

In the following code example ellipsis `...` represents omitted code. `TypeParam` takes the next type of `scalar_t`
```cpp
template <typename T> class H5ETest : public AbstractTest<T>{};
using scalar_t = ::testing::Types<char, int, long, long long, float, double, ...>;
TYPED_TEST_CASE(H5ETest, scalar_t);

TYPED_TEST(H5ETest, cartesian_example) { //tets case iterated through `scalar_t`
	using tuple_t = std::tuple<std::vector<TypeParam>, std::array<TypeParam,3>>;
	tuple_t data = h5::mock::data<tuple_t>( 1024 ); // obtain dataset of tuple_t type
	// the cartesian product  {elementary_t X tuple_t} are executed within lambda
	h5::meta::static_for<tuple_t>( [&]( auto i ){
        auto v = std::get<i>( data ); // get tuple element at compile time
		...
		ASSERT_TRUE(...);
    });
	// implement test cases for scalars outside of loop
	...
}
```
`AbstractTest<T> : public ::testing::Test { ... }` class sets up the environment for HDF5 IO test, is defined in `h5cpp-test.hpp` and provides additional functionality such as defining dataset names as a function of executed datatype
```cpp
template <typename T> class AbstractTest
					: public ::testing::Test {
	using container_t = std::tuple<std::array<T,10>, std::vector<T>, std::deque<T>>;
public:
	void SetUp() {
		dir = ::testing::UnitTest::GetInstance()->current_test_info()->name();
		type = h5::name<T>::value;
		name = dir + "/" + type;
		this->fd = h5::open("test.h5", H5F_ACC_RDWR );
	}
	void TearDown() {
	}

	std::string type;
	std::string name;
	std::string dir;
	h5::fd_t fd; //< file descriptor
};
```

In addition to test case assertions there is a general error handler which triggers failure if HDF5 CAPI reports error. Consider this as a fail safe switch to prevent erroneous implementation pass.
```
herr_t CAPI_error_handler (long int, void *) {
	hid_t error_stack = H5Eget_current_stack();
	H5Eclear( error_stack );
	ADD_FAILURE();
	return error_stack;
}
```

And finally the output with a custom printer. For `json | xml | ... ` take a look at google test framework.
```bash
--------------------------------------------------------------------------------------
 RUNNING TEST SUITE:                                                           ./H5E 
--------------------------------------------------------------------------------------
cartesian_example                                              unsigned char [  OK  ]
cartesian_example                                             unsigned short [  OK  ]
cartesian_example                                               unsigned int [  OK  ]
cartesian_example                                         unsigned long long [  OK  ]
cartesian_example                                                       char [  OK  ]
cartesian_example                                                      short [  OK  ]
cartesian_example                                                        int [  OK  ]
cartesian_example                                                  long long [  OK  ]
cartesian_example                                                      float [  OK  ]
cartesian_example                                                     double [  OK  ]
-------------------------------------------------------------------------------------- 
PASSED: 10  FAILED: 0 TIME: 3 ms 
-------------------------------------------------------------------------------------
```
## Mock Functions
* `std::array<T,N> mock::randu(size_t min, size_t max)` returns an array of T where `T ::= numerical | std::string`
* `std::tuple<...> mock::data<std::tuple<...>>(size_t min, size_t max, double fill_rate)` return tuple of random length and content of datasets with cardinality specified by `min` and `max`. 

# Prerequisites 

### Basics
```shell
apt install build-essential
apt install google-perftools kcachegrind
```

### HDF5 C library
Download and compile [the latest version][207] from The HDFGroup website or use the provided [binary packages][206]. The minimum version requirements for HDF5 CAPI is set to v1.10.2.



### Compiler
[GCC 7.0][gcc] or above
[clang 5.0][clang] or above
with `-std=c++17`

### Linear Algebra
optionally install linear algebra packages from source, or package manager
```shell
apt install libarmadillo-dev libeigen3-dev libblitz0-dev libitpp-dev libdlib-dev libboost-all-dev 
```
In acase of [blaze][100] and  header files to `/usr/local/include`, and [ETL][101] is slightly trickier, make sure to clone it recursively, and have minimum version of gcc 6.3.0 / clang 3.9

`git clone --recursive https://github.com/wichtounet/etl.git`
`cd etl; CXX=clang++ make`

### Google Test
is used to test integrity, here are the steps:

1. install google-test sources: `sudo apt-get install libgtest-dev`
2. get cmake: `sudo apt-get install cmake`
3. do `cd /usr/src/gtest` to enter the source directory
4. `sudo cmake CMakeLists.txt && make`


[gcc]: https://gcc.gnu.org/projects/cxx-status.html#cxx14
[clang]: https://clang.llvm.org/cxx_status.html

[100]: https://bitbucket.org/blaze-lib/blaze/src/master/
[101]: https://github.com/wichtounet/etl
[206]: https://www.hdfgroup.org/downloads/hdf5/
[207]: https://www.hdfgroup.org/downloads/hdf5/source-code/

