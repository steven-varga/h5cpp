#  _____________________________________________________________________________
#  Copyright (c) <2019> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2019 Toronto, On Canada
#  _____________________________________________________________________________

cmake_minimum_required(VERSION 3.12.4 FATAL_ERROR)
if(${CMAKE_VERSION} VERSION_GREATER "3.17.0")
	cmake_policy(SET CMP0074 NEW)
endif()
## match hdf5 versioning x.x.x.h5cpp-version
project(libh5cpp-dev VERSION 1.10.4.5 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
find_package( Threads REQUIRED QUIET)

add_library(h5cpp INTERFACE)
# check if the correct version of hdf5 available
set(H5CPP_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
if(WIN32)
	set(CMAKE_PREFIX_PATH  "C:/linalg"  "C:/linalg/lib"  "C:/linalg/lib/cmake"  "C:/linalg/share")
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/"
			"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )
	set(ARMADILLO_LIBRARY_DIRS  "C:/linalg/bin")

elseif(UNIX)
	set(HDF5_ROOT /usr/local/ /usr) # there maybe more upto date library in these places
	include(GNUInstallDirs)
	find_package(PkgConfig REQUIRED QUIET)
endif()
if(APPLE)
	message(STATUS "***WARNING: h5cpp is not fully tested on apple ***")
endif()
if(ANDROID)
	message(STATUS "***WARNING: h5cpp is not fully tested on android ***")
endif()

include(FindHDF5)
if(HDF5_VERSION VERSION_LESS ${H5CPP_VERSION})
	message(FATAL_ERROR "-- !!! H5CPP examples require of HDF5 v${H5CPP_VERSION} or greater!!!")
else()
	message("-- H5CPP ${PROJECT_VERSION} matches with minimum required HDF5 v${HDF5_VERSION}")
endif()

if(HDF5_VERSION VERSION_GREATER 1.12.0)
	message("H5CPP KITA support is available, see webpage for details...")
endif()

include(FindMPI MPI_SKIP_COMPILER_WRAPPER)
if( MPI_FOUND AND HDF5_IS_PARALLEL )
	message("-- MPI and PHDF5 found: Parallel H5CPP enabled")
endif()


#add_subdirectory(examples)
find_package(GTest)
find_package(fmt)
if(GTest_FOUND)
	option(H5CPP_BUILD_TESTS "Build tests" ON)
elseif()
	option(H5CPP_BUILD_TESTS "Build tests" OFF)
endif()

#option(H5CPP_BUILD_TESTS "Build tests" OFF)
if(H5CPP_BUILD_TESTS)
	message("-- gtest is enabled, optionally execute: `make test`")
    enable_testing()
    add_subdirectory(tests)
endif()


#installation
target_include_directories(h5cpp INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)

if(WIN32)
	if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
		set (CMAKE_INSTALL_PREFIX "C:/linalg" CACHE PATH "default install path" FORCE )
	endif()
	set(INSTALL_DIR_CMAKE "${CMAKE_INSTALL_PREFIX}/share/h5cpp/cmake")
	set(INSTALL_DIR_INCLUDE  "${CMAKE_INSTALL_PREFIX}/include")
	set(INSTALL_DIR_EXAMPLES "${CMAKE_INSTALL_PREFIX}/examples")
elseif(UNIX)
	set(INSTALL_DIR_CMAKE    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/h5cpp")
	set(INSTALL_DIR_INCLUDE  "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
	set(INSTALL_DIR_EXAMPLES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp")
endif()

message( "-- Install directory: ${INSTALL_DIR_INCLUDE}")

# copy header only library into DESTDIR
install(DIRECTORY h5cpp DESTINATION ${INSTALL_DIR_INCLUDE})
install(TARGETS h5cpp EXPORT h5cpp-targets)

install(DIRECTORY examples/attributes     DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/basics         DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/before-after   DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/compound       DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/linalg         DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/multi-tu       DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/optimized      DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/packet-table   DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/raw_memory     DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/transform      DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(FILES     examples/CMakeLists.txt DESTINATION "${INSTALL_DIR_EXAMPLES}")

#generate cmake config and version files
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/config.cmake.in
    ${CMAKE_BINARY_DIR}/h5cpp-config.cmake
    INSTALL_DESTINATION ${INSTALL_DIR_CMAKE}
)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/h5cpp-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

export(PACKAGE h5cpp)
export(EXPORT h5cpp-targets)

install(EXPORT h5cpp-targets DESTINATION ${INSTALL_DIR_CMAKE})
install(FILES
    ${CMAKE_BINARY_DIR}/h5cpp-config.cmake
    ${CMAKE_BINARY_DIR}/h5cpp-config-version.cmake
    DESTINATION ${INSTALL_DIR_CMAKE}
)

