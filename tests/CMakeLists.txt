#  _____________________________________________________________________________
#  Copyright (c) <2018-2020> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018-2020 Toronto, On Canada
#  _____________________________________________________________________________

enable_testing()
include(GoogleTest)
find_package(ZLIB REQUIRED)


list(APPEND CMAKE_INCLUDE_PATH "/usr/include")
list(APPEND CMAKE_INCLUDE_PATH "/usr/local/include" )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/"
	"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )

find_package(dlib QUIET)
find_package(Eigen3 QUIET)
find_package(Armadillo QUIET)
find_package(Blitz QUIET)
find_package(blaze QUIET)
find_package(ublas QUIET)

pkg_check_modules(ITPP QUIET itpp)
pkg_check_modules(Blitz QUIET blitz)

# propagate macro definitions into source files
# -DNAME_FOUND=0|1
function(add_def name)
	string(TOUPPER ${name} macro)
	if( ${name}_FOUND )
		add_definitions(-DH5CPP_USE_${macro})
	endif()
endfunction()

add_def(EIGEN3)
add_def(ARMADILLO)
add_def(dlib)
add_def(blaze)
add_def(ITPP)
add_def(Blitz)
add_def(UBLAS)

# define test case with dependencies
function(h5cpp_test target)
    add_executable(test-${target} ${target}.cpp)
	set_property(TARGET test-${target} PROPERTY CXX_STANDARD 17)
	target_include_directories(test-${target} PRIVATE ${HDF5_C_INCLUDE_DIRS} Gtest::Gtest Eigen3::Eigen)
	target_link_libraries(test-${target} ${HDF5_LIBRARIES} GTest::GTest GTest::Main Eigen3::Eigen dlib::dlib fmt::fmt)
	gtest_discover_tests(test-${target})
endfunction()
# test cases are lifted out from the following test programs
h5cpp_test(H5cmake)
h5cpp_test(H5Tmeta)

