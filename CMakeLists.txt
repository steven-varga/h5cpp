#  _____________________________________________________________________________
#  Copyright (c) <2018-2020> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018-2020 Toronto, On Canada
#  _____________________________________________________________________________



cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
## match hdf5 versioning x.x.x.h5cpp-version
project(libh5cpp-dev VERSION 1.10.4.5 LANGUAGES CXX C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/"
	"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )
set(CMAKE_CXX_STANDARD 17)

add_library(h5cpp INTERFACE)

# check if the correct version of hdf5 available
set(H5CPP_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(HDF5_ROOT /usr/local/ /usr) # there maybe more upto date library in these places
include(FindHDF5)
if(HDF5_VERSION VERSION_LESS ${H5CPP_VERSION})
	message(FATAL_ERROR "-- !!! H5CPP examples require of HDF5 v${H5CPP_VERSION} or greater!!!")
else()
	message("-- H5CPP ${PROJECT_VERSION} matches with minimum required HDF5 v${HDF5_VERSION}")
endif()

if(HDF5_VERSION VERSION_GREATER 1.12.0)
	message("H5CPP KITA support is available, see webpage for details...")
endif()

include(FindMPI)
if( MPI_FOUND AND HDF5_IS_PARALLEL )
	message("-- MPI and PHDF5 found: Parallel H5CPP enabled")
endif()

if(WIN32)
	message(STATUS "***WARNING: h5cpp is not fully tested on windows, use this port: https://github.com/ChrisDrozdowski/h5cpp ***")
endif()
if(APPLE)
	message(STATUS "***WARNING: h5cpp is not fully tested on apple ***")
endif()
if(ANDROID)
	message(STATUS "***WARNING: h5cpp currently doesn't work on 32bit systems nor clang > 6.0.0 ***")
	message(STATUS "***WARNING: cross compiling HDF5 CAPI will not work, instead either download the binary from ***")
	message(STATUS "***WARNING: authors github page, or compile the libhdf5 on device | in emulator then tarball the ***")
	message(STATUS "***WARNING: binaries, copy them back to host, link against the libraries, enjoy the coffee.... ***")
endif()

include(GNUInstallDirs)
find_package(Doxygen QUIET)

#add_subdirectory(examples)
option(H5CPP_BUILD_TESTS "Build tests" OFF)
if(H5CPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()


#installation
target_include_directories(h5cpp INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
)

set(INSTALL_DIR_CMAKE    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/h5cpp")
set(INSTALL_DIR_INCLUDE  "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
set(INSTALL_DIR_EXAMPLES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp")

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

