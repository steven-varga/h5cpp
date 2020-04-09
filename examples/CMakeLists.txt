#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

## match hdf5 versioning x.x.x.h5cpp-version
project(h5cpp-examples VERSION 1.10.4.5 LANGUAGES CXX C)

# check if the correct version of hdf5 available
set(H5CPP_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(HDF5_FIND_DEBUG FALSE)
include(FindHDF5)
if(HDF5_VERSION VERSION_LESS ${H5CPP_VERSION})
	message(FATAL_ERROR "H5CPP examples require of HDF5 v${H5CPP_VERSION} or greater!!!")
else()
	message("-- H5CPP ${PROJECT_VERSION} matches with minimum required HDF5 v${HDF5_VERSION}")
endif()
if(HDF5_VERSION VERSION_GREATER 1.12.0)
	message("H5CPP KITA support is available, see webpage for details...")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/"
	"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )
set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	message(STATUS "***WARNING: h5cpp is not fully tested on windows, but know to work ***")
endif()
if(APPLE)
	message(STATUS "***WARNING: h5cpp is not fully tested on apple ***")
endif()

include(GNUInstallDirs)
find_package(Doxygen QUIET)

list(APPEND CMAKE_INCLUDE_PATH "/usr/include")
list(APPEND CMAKE_INCLUDE_PATH "/usr/local/include" )
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/" 
	"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )

find_program(H5CPP 
	NAME h5cpp
	HINTS /usr/bin /usr/local/bin
	DOC "path to h5cpp source code transformation tool")

if( H5CPP )
	message("-- H5CPP compiler found: ${H5CPP}" )
	set(H5CPP_COMPILER_FOUND TRUE)
else()
	set(H5CPP_COMPILER_FOUND FALSE)
endif()

#find_package(HDF5 1.10.4 REQUIRED QUIET)
include(FindHDF5)
if(HDF5_VERSION VERSION_LESS 1.10.4)
	message(FATAL_ERROR "H5CPP examples require HDF5 minimum version is 1.10.4")
endif()

if(WIN32)
	set(CMAKE_PREFIX_PATH  "C:/linalg"  "C:/linalg/lib"  "C:/linalg/lib/cmake"  "C:/linalg/share")
	set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/"
			"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )
	set(ARMADILLO_LIBRARY_DIRS  "C:/linalg/bin")

elseif(UNIX)
	set(HDF5_CMAKE_CURRENT_SOURCE_DIR /usr/local/ /usr) # there maybe more upto date library in these places
	include(GNUInstallDirs)
	find_package(PkgConfig REQUIRED QUIET)
endif()

find_package(ZLIB REQUIRED QUIET)

find_package(dlib QUIET)
find_package(Eigen3 QUIET)
find_package(Armadillo QUIET)
find_package(blaze QUIET)
find_package(ublas QUIET )
find_package(Boost QUIET)


# these ones are pkg-config based
pkg_check_modules(ITPP QUIET itpp)
pkg_check_modules(Blitz QUIET blitz)

list(APPEND LIBS ${HDF5_C_LIBRARIES} ${ZLIB_LIBRARIES})
list(APPEND INCLUDE_DIRS ${CMAKE_SOURCE_DIR} ${HDF5_INCLUDE_DIRS}  ${ZLIB_INCLUDE_DIRS})

include_directories(BEFORE ${CMAKE_SOURCE_DIR})

if( H5CPP_COMPILER_FOUND )
	add_custom_target(h5cpp-compiler-generated ALL
		COMMAND	${H5CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/attributes/attributes.cpp -- ${CMAKE_CXXFLAGS} -D${CMAKE_CURRENT_SOURCE_DIR}/attributes/generated.h
		COMMAND	${H5CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/before-after/compound.cpp -- ${CMAKE_CXXFLAGS} -D${CMAKE_CURRENT_SOURCE_DIR}/before-after/generated.h
		COMMAND	${H5CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/compound/struct.cpp -- ${CMAKE_CXXFLAGS} -D${CMAKE_CURRENT_SOURCE_DIR}/compound/generated.h
		COMMAND	${H5CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/multi-tu/tu-01.cpp -- ${CMAKE_CXXFLAGS} -D${CMAKE_CURRENT_SOURCE_DIR}/multi-tu/tu-01.h
		COMMAND	${H5CPP}  ${CMAKE_CURRENT_SOURCE_DIR}/multi-tu/tu-02.cpp -- ${CMAKE_CXXFLAGS} -D${CMAKE_CURRENT_SOURCE_DIR}/multi-tu/tu-02.h
		COMMENT "-- generating hdf5 compound type descriptors with h5cpp compiler ...")
	if( ${Eigen3_FOUND} )
		add_custom_target(h5cpp-compiler-generated-eigen ALL
			${H5CPP} ${CMAKE_CURRENT_SOURCE_DIR}/packet-table/packettable.cpp -- 
			${CMAKE_CXXFLAGS}  -I${EIGEN3_INCLUDE_DIR} -D${CMAKE_CURRENT_SOURCE_DIR}/packet-table/generated.h
		COMMENT "-- generating hdf5 compound type descriptors with h5cpp compiler ...")
	endif()
	add_custom_target(examples ALL DEPENDS h5cpp-compiler-generated h5cpp-compiler-generated-eigen )
else()
	add_custom_target(examples ALL)
	message("-- H5CPP compiler NOT found: using `generated.h` already present...")
endif()

if( ${dlib_FOUND} )
	message( "-- Found: dlib library, added to examples" )
	add_executable(examples-dlib EXCLUDE_FROM_ALL "linalg/dlib.cpp")
	# dlib doesn't like c++17
	target_compile_options(examples-dlib PRIVATE "-w")
	target_link_libraries(examples-dlib ${LIBS} dlib::dlib)
	set_property(TARGET examples-dlib PROPERTY CXX_STANDARD 14)
	add_dependencies(examples examples-dlib)
endif()

if( ${Eigen3_FOUND} )
	message( "-- Found: eigen3 ${Eigen3_VERSION} added to examples" )
	add_executable(examples-eigen3 EXCLUDE_FROM_ALL "linalg/eigen3.cpp")
	include_directories(${EIGEN3_INCLUDE_DIR})
	target_link_libraries(examples-eigen3 ${LIBS} Eigen3::Eigen )
	add_dependencies(examples examples-eigen3)
endif()

if( ${ARMADILLO_FOUND} )
	message( "-- Found: armadillo ${ARMADILLO_VERSION_STRING} added to examples" )
	add_executable(examples-arma EXCLUDE_FROM_ALL "linalg/arma.cpp")
	target_link_libraries(examples-arma ${LIBS})
	add_dependencies(examples examples-arma)
endif()

if( ${blaze_FOUND} )
	message( "-- Found: blaze ${blaze_VERSION} added to examples" )
	add_executable(examples-blaze EXCLUDE_FROM_ALL "linalg/blaze.cpp")
	target_link_libraries(examples-blaze ${LIBS})
	add_dependencies(examples examples-blaze)
endif()

if( ${ITPP_FOUND} )
	message( "-- Found: itpp ${ITPP_VERSION}, added to examples" )
	add_executable(examples-itpp EXCLUDE_FROM_ALL "linalg/itpp.cpp")
	target_link_libraries(examples-itpp ${LIBS}  ${ITPP_LDFLAGS}  )
	add_dependencies(examples examples-itpp)
endif()

if( ${Blitz_FOUND} )
	message( "-- Found: blitz ${Blitz_VERSION}, added to examples" )
	#add_executable(examples-blitz EXCLUDE_FROM_ALL "linalg/blitz.cpp")
	#target_link_libraries(examples-blitz ${LIBS}  ${Blitz_LDFLAGS} )
	#TODO: re-add example for blitz++ 
	#add_dependencies(examples examples-blitz)
endif()

if( ${Boost_FOUND} )
	if(EXISTS "${Boost_INCLUDE_DIRS}/boost/numeric/ublas")
		set(ublas_DIR "${Boost_INCLUDE_DIRS}/boost/numeric/ublas" CACHE PATH "path to ublas includes" FORCE)
		message( "-- Found: ublas ${Boost_VERSION}" )
		add_executable(examples-ublas EXCLUDE_FROM_ALL "linalg/ublas.cpp")
		target_link_libraries(examples-ublas ${LIBS} )
		add_dependencies(examples examples-ublas)
	endif()
endif()

include(FindMPI)
if( MPI_FOUND AND HDF5_IS_PARALLEL )
	find_program(SLURM_SBATCH_COMMAND sbatch DOC "Path to the SLURM sbatch executable")
	find_program(SLURM_SRUN_COMMAND srun DOC "Path to the SLURM srun executable")
	find_program(SLURM_SACCTMGR_COMMAND sacctmgr DOC "Path to the SLURM sacctmgr executable")

	message("-- MPI and PHDF5 found: Parallel H5CPP enabled")
	message("-- using ${MPI_CXX_COMPILER} ${MPIEXEC_EXECUTABLE}")
	if(SLURM_SRUN_COMMAND AND SLURM_SBATCH_COMMAND)
		set(SLURM_FOUND true)
	endif()
	if( SLURM_FOUND )
		message("-- SLURM found, scheduling with ${SLURM_SBATCH_COMMAND} ${SLURM_SRUN_COMMAND}")
	elseif()
		message("-- SLURM NOT FOUND schedule the test with mpiexec -n proc_num ...")
	endif()
	## mpi-collective ###
	add_executable(examples-mpi-collective EXCLUDE_FROM_ALL "mpi/collective.cpp")
	target_link_libraries(examples-mpi-collective ${LIBS} ${MPI_CXX_LIBRARIES} )
	target_compile_definitions(examples-mpi-collective PUBLIC ${MPI_CXX_COMPILE_DEFINITIONS})
	target_include_directories(examples-mpi-collective PUBLIC ${MPI_CXX_INCLUDE_DIRS})
	add_dependencies(examples examples-mpi-collective)

	## mpi-independent ###
	add_executable(examples-mpi-independent EXCLUDE_FROM_ALL "mpi/independent.cpp")
	target_link_libraries(examples-mpi-independent ${LIBS} ${MPI_CXX_LIBRARIES} )
	target_compile_definitions(examples-mpi-independent PUBLIC ${MPI_CXX_COMPILE_DEFINITIONS})
	target_include_directories(examples-mpi-independent PUBLIC ${MPI_CXX_INCLUDE_DIRS})
	add_dependencies(examples examples-mpi-independent)

	## mpi throughput
	add_executable(examples-mpi-throughput EXCLUDE_FROM_ALL "mpi/throughput.cpp")
	target_link_libraries(examples-mpi-throughput ${LIBS} ${MPI_CXX_LIBRARIES} )
	target_compile_definitions(examples-mpi-throughput PUBLIC ${MPI_CXX_COMPILE_DEFINITIONS})
	target_include_directories(examples-mpi-throughput PUBLIC ${MPI_CXX_INCLUDE_DIRS})
	add_dependencies(examples examples-mpi-throughput)


endif()

## attributes ###
add_executable(examples-attributes EXCLUDE_FROM_ALL "attributes/attributes.cpp")
target_link_libraries(examples-attributes ${LIBS} )
add_dependencies(examples examples-attributes)

## basics ###
add_executable(examples-basics EXCLUDE_FROM_ALL "basics/basics.cpp")
target_link_libraries(examples-basics ${LIBS} )
add_dependencies(examples examples-basics)


## before-after: before ###
add_executable(examples-before EXCLUDE_FROM_ALL "before-after/compound.c")
target_link_libraries(examples-before ${LIBS} )
add_dependencies(examples examples-before)

## before-after: after ###
add_executable(examples-after EXCLUDE_FROM_ALL "before-after/compound.cpp")
target_link_libraries(examples-after ${LIBS} )
add_dependencies(examples examples-after)

## compound ###
add_executable(examples-compound EXCLUDE_FROM_ALL "compound/struct.cpp")
target_link_libraries(examples-compound ${LIBS} )
add_dependencies(examples examples-compound)

## csv ###
add_executable(examples-csv EXCLUDE_FROM_ALL "csv/csv2hdf5.cpp")
target_link_libraries(examples-csv ${LIBS} Threads::Threads )
add_dependencies(examples examples-csv)

## datasets ###
add_executable(examples-datasets EXCLUDE_FROM_ALL "datasets/datasets.cpp")
target_link_libraries(examples-datasets ${LIBS} )
add_dependencies(examples examples-datasets)

if( ${Eigen3_FOUND} )
## datatypes nbit ###
	add_executable(examples-nbit EXCLUDE_FROM_ALL "datatypes/n-bit.cpp")
	target_link_libraries(examples-nbit ${LIBS}  )
	include_directories(${EIGEN3_INCLUDE_DIR})
	add_dependencies(examples examples-nbit)
endif()

## datatypes twobit ###
add_executable(examples-twobit EXCLUDE_FROM_ALL "datatypes/two-bit.cpp")
target_link_libraries(examples-twobit ${LIBS} )
add_dependencies(examples examples-twobit)

## multi translation unit ###
#add_library(examples-multi-tu01 OBJECT "multi-tu/tu_01.cpp")
#add_library(examples-multi-tu02 OBJECT "multi-tu/tu_02.cpp")

#add_executable(examples-multi-tu EXCLUDE_FROM_ALL "multi-tu/main.cpp")
#target_link_libraries(examples-multi-tu  examples-multi-tu01 examples-multi-tu02 ${LIBS}   )
#add_dependencies(examples examples-multi-tu)

## optimized ###
add_executable(examples-optimized EXCLUDE_FROM_ALL "optimized/optimized.cpp")
target_link_libraries(examples-optimized ${LIBS} )
add_dependencies(examples examples-optimized)

## packet-table ###
add_executable(examples-packet-table EXCLUDE_FROM_ALL "packet-table/packettable.cpp")
target_link_libraries(examples-optimized ${LIBS} )
add_dependencies(examples examples-optimized)

## raw memory ###
add_executable(examples-raw EXCLUDE_FROM_ALL "raw_memory/raw.cpp")
target_link_libraries(examples-raw ${LIBS} )
add_dependencies(examples examples-raw)

## stl ###
add_executable(examples-stl EXCLUDE_FROM_ALL "stl/vector.cpp")
target_link_libraries(examples-stl ${LIBS} )
add_dependencies(examples examples-stl)

## string ###
add_executable(examples-string EXCLUDE_FROM_ALL "string/string.cpp")
target_link_libraries(examples-string ${LIBS} )
add_dependencies(examples examples-string)

## transform ###
add_executable(examples-transform EXCLUDE_FROM_ALL "transform/transform.cpp")
target_link_libraries(examples-transform ${LIBS} )
add_dependencies(examples examples-transform)

## utf ###
add_executable(examples-utf EXCLUDE_FROM_ALL "utf/utf.cpp")
target_link_libraries(examples-utf ${LIBS} )
add_dependencies(examples examples-utf)
