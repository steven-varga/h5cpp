#  _________________________________________________________
#  Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________
cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

## match hdf5 versioning x.x.x.h5cpp-version
project(h5cpp-examples VERSION 1.10.4.5 LANGUAGES CXX C)

# Honor <PackageName>_ROOT (CMP0074, CMake 3.12+). The cmake_minimum_required
# above predates it, so without this find_package(HDF5) below would silently
# ignore the HDF5_ROOT we build for custom HDF Group installs. (#295)
if(POLICY CMP0074)
	cmake_policy(SET CMP0074 NEW)
endif()

# check if the correct version of hdf5 available
set(H5CPP_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# Mirror the top-level CMakeLists' HDF5 search-path policy: pick up custom
# HDF Group installs at /usr/local/HDF_Group/HDF5/<version>/ before system
# paths. Needed when the examples sub-project is configured standalone — if
# we relied solely on the parent setting HDF5_ROOT, a standalone configure
# would silently fall through to /usr/local/lib/libhdf5.so. When examples is
# added via add_subdirectory() the parent's HDF5_ROOT is already in scope
# and the union (custom + /usr/local + /usr) reaches find_package correctly.
if(UNIX AND NOT DEFINED HDF5_ROOT)
	file(GLOB _hdf5_custom_roots LIST_DIRECTORIES true
		"/usr/local/HDF_Group/HDF5/[0-9]*")
	list(SORT _hdf5_custom_roots ORDER DESCENDING)
	set(HDF5_ROOT ${_hdf5_custom_roots} "/usr/local" "/usr")
endif()

find_package(HDF5 REQUIRED COMPONENTS C)

if(HDF5_VERSION VERSION_LESS ${H5CPP_VERSION})
	message(FATAL_ERROR "H5CPP examples require of HDF5 v${H5CPP_VERSION} or greater!!!")
else()
	message("-- H5CPP ${PROJECT_VERSION} matches with minimum required HDF5 v${HDF5_VERSION}")
endif()
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
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
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

find_program(H5CPP NAME h5cpp HINTS /usr/bin /usr/local/bin DOC "path to h5cpp source code transformation tool")
if( H5CPP )
	message("-- H5CPP compiler found: ${H5CPP}" )
	set(H5CPP_COMPILER_FOUND TRUE)
else()
	set(H5CPP_COMPILER_FOUND FALSE)
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

# Vendored linear algebra third-party dependencies
if(NOT TARGET libeigen3)
	if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
		add_subdirectory(thirdparty)
	elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty)
		add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty ${CMAKE_CURRENT_BINARY_DIR}/thirdparty)
	else()
		message(FATAL_ERROR "thirdparty directory not found")
	endif()
endif()

# Expose find_package-style variables from vendored targets so the conditional
# example blocks below work regardless of whether the libraries were found via
# find_package or via the vendored thirdparty subdirectory.
if(TARGET libarmadillo)
    set(ARMADILLO_FOUND TRUE)
endif()
if(TARGET libeigen3)
    set(Eigen3_FOUND TRUE)
endif()

list(APPEND LIBS ${HDF5_C_LIBRARIES} ZLIB::ZLIB libh5cpp-libdeflate)
list(APPEND INCLUDE_DIRS ${CMAKE_SOURCE_DIR} ${HDF5_INCLUDE_DIRS}  ${ZLIB_INCLUDE_DIRS})
include_directories(BEFORE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/.. ${HDF5_C_INCLUDE_DIRS})
if(NOT H5CPP_COMPILER_FOUND)
	message("-- H5CPP compiler NOT found: using `generated.h` already present...")
endif()

# ------------------------------------------------------------------------------
# Centralised helper for example executables
# ------------------------------------------------------------------------------
## usage:
##   add_h5cpp_example(<target-suffix> <source-path>
##       [GENERATED <header-path>]
##       [LIBRARIES <lib>...]
##       [COMPILER_INCLUDES <dir>...]
##       [COMPILER_FLAGS <flag>...]
##       [COMPILE_DEFINITIONS <def>...]
##       [INCLUDE_DIRECTORIES <dir>...]
##   )
function(add_h5cpp_example target_suffix source_path)
	set(options)
	set(one_value_args GENERATED)
	set(multi_value_args LIBRARIES COMPILER_INCLUDES COMPILER_FLAGS COMPILER_SOURCES COMPILE_DEFINITIONS INCLUDE_DIRECTORIES)
	cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

	set(target_name examples-${target_suffix})
	set(source_file ${CMAKE_CURRENT_SOURCE_DIR}/${source_path})

	# Ensure aggregate target exists (first call creates it)
	if(NOT TARGET examples)
		add_custom_target(examples ALL)
	endif()

	# ---- generated header handling ----
	if(ARG_GENERATED)
		set(generated_file ${CMAKE_CURRENT_SOURCE_DIR}/${ARG_GENERATED})
		if(H5CPP_COMPILER_FOUND)
			set(h5cpp_args "-o" "${generated_file}")
			foreach(inc IN LISTS ARG_COMPILER_INCLUDES)
				list(APPEND h5cpp_args "--extra-arg=-I${inc}")
			endforeach()
			foreach(flag IN LISTS ARG_COMPILER_FLAGS)
				list(APPEND h5cpp_args "--extra-arg=${flag}")
			endforeach()

			set(compiler_inputs ${source_file})
			foreach(src IN LISTS ARG_COMPILER_SOURCES)
				list(APPEND compiler_inputs ${CMAKE_CURRENT_SOURCE_DIR}/${src})
			endforeach()

			get_filename_component(generated_name ${ARG_GENERATED} NAME_WE)
			set(generated_target ${target_name}-generated-${generated_name})

			add_custom_command(
				OUTPUT  ${generated_file}
				COMMAND ${H5CPP} ${h5cpp_args} ${compiler_inputs}
				DEPENDS ${compiler_inputs}
				COMMENT "Generating ${ARG_GENERATED} for ${target_name}"
				VERBATIM
			)
			add_custom_target(${generated_target} DEPENDS ${generated_file})
			set(generated_dep ${generated_target})
		else()
			message(STATUS "H5CPP compiler not found: assuming ${generated_file} is present")
		endif()
	endif()

	# ---- executable ----
	add_executable(${target_name} EXCLUDE_FROM_ALL ${source_file})
	target_link_libraries(${target_name} PRIVATE ${LIBS} ${ARG_LIBRARIES})
	target_compile_options(${target_name} PRIVATE "-w")
	# Drop the `examples/` path prefix from the binary's output location so the
	# executables land in the top-level build directory: <build>/examples-<name>
	# instead of <build>/examples/examples-<name>. Run as `./examples-<name>`.
	set_target_properties(${target_name} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

	if(ARG_COMPILE_DEFINITIONS)
		target_compile_definitions(${target_name} PUBLIC ${ARG_COMPILE_DEFINITIONS})
	endif()
	if(ARG_INCLUDE_DIRECTORIES)
		target_include_directories(${target_name} PUBLIC ${ARG_INCLUDE_DIRECTORIES})
	endif()

	if(generated_dep)
		add_dependencies(${target_name} ${generated_dep})
	endif()
	add_dependencies(examples ${target_name})
endfunction()

# ------------------------------------------------------------------------------
# Linalg examples
# ------------------------------------------------------------------------------
message( "-- Vendored: dlib library, added to examples" )
add_h5cpp_example(dlib linalg/dlib.cpp LIBRARIES libdlib)

message( "-- Vendored: eigen3 added to examples" )
add_h5cpp_example(eigen3 linalg/eigen3.cpp LIBRARIES Eigen3::Eigen)

message( "-- Vendored: armadillo added to examples" )
add_h5cpp_example(arma linalg/arma.cpp LIBRARIES libarmadillo)

message( "-- Vendored: blaze added to examples" )
add_h5cpp_example(blaze linalg/blaze.cpp LIBRARIES libblaze)

# NOTE: itpp example disabled — upstream ITPP v4.3.1 is not C++17 compatible
# (uses removed 'register' keyword). Fails with GCC 14. Not an h5cpp bug.
# message( "-- Vendored: itpp added to examples" )
# add_h5cpp_example(itpp linalg/itpp.cpp LIBRARIES libitpp)

message( "-- Vendored: blitz added to examples" )
add_h5cpp_example(blitz linalg/blitz.cpp LIBRARIES libblitz)

message( "-- Vendored: xtensor added to examples" )
add_h5cpp_example(xtensor linalg/xtensor.cpp LIBRARIES libxtensor)

find_package(BLAS)
if(BLAS_FOUND AND XFLENS_FOUND)
    message( "-- Vendored: xtensor-blas added to examples" )
    add_h5cpp_example(xtensor-blas linalg/xtensor-blas.cpp LIBRARIES libxtensor-blas ${BLAS_LIBRARIES})
else()
    if(BLAS_FOUND)
        message("-- xflens not found — xtensor-blas example disabled")
    else()
        message("-- BLAS not found — xtensor-blas example disabled")
    endif()
endif()

# uBLAS is fully vendored (header-only closure under thirdparty/ublas, #293) —
# libublas resolves <boost/numeric/ublas/...> standalone, so no system Boost is
# required and the example always builds.
message("-- Vendored: ublas added to examples")
add_h5cpp_example(ublas linalg/ublas.cpp LIBRARIES libublas)

# ------------------------------------------------------------------------------
# Sparse linalg examples (CSC group layout, scipy/Julia interop)
# ------------------------------------------------------------------------------
if(${ARMADILLO_FOUND})
    add_h5cpp_example(sparse-arma sparse/arma.cpp LIBRARIES libarmadillo)
endif()
add_h5cpp_example(sparse-eigen sparse/eigen.cpp LIBRARIES Eigen3::Eigen)

# ------------------------------------------------------------------------------
# MPI examples
# ------------------------------------------------------------------------------
# Two tiers:
#   * Parallel HDF5 (collective / independent / throughput) — need HDF5 built
#     with --enable-parallel.  Gated on HDF5_IS_PARALLEL.
#   * MPI + serial HDF5 (file_per_rank) — needs only MPI; runs on any HDF5
#     build, no parallel filesystem required.
include(FindMPI)

if( MPI_FOUND )
	find_program(SLURM_SBATCH_COMMAND sbatch DOC "Path to the SLURM sbatch executable")
	find_program(SLURM_SRUN_COMMAND srun DOC "Path to the SLURM srun executable")
	find_program(SLURM_SACCTMGR_COMMAND sacctmgr DOC "Path to the SLURM sacctmgr executable")
	if(SLURM_SRUN_COMMAND AND SLURM_SBATCH_COMMAND)
		set(SLURM_FOUND true)
	endif()

	message("-- MPI found: ${MPI_CXX_COMPILER} (mpiexec: ${MPIEXEC_EXECUTABLE})")
	if( SLURM_FOUND )
		message("-- SLURM found: schedule with ${SLURM_SBATCH_COMMAND} / ${SLURM_SRUN_COMMAND}")
	else()
		message("-- SLURM not found: run with mpirun -n <N> ./examples-mpi-<name>")
	endif()

	# ── Tier 1: MPI + serial HDF5 — builds on any HDF5 install ───────────
	add_h5cpp_example(mpi-file-per-rank mpi/file_per_rank.cpp
		LIBRARIES ${MPI_CXX_LIBRARIES}
		COMPILE_DEFINITIONS ${MPI_CXX_COMPILE_DEFINITIONS}
		INCLUDE_DIRECTORIES ${MPI_CXX_INCLUDE_DIRS})

	# ── Tier 2: parallel HDF5 — requires --enable-parallel build ─────────
	if( HDF5_IS_PARALLEL )
		message("-- Parallel HDF5 found: collective / independent / throughput enabled")
		add_h5cpp_example(mpi-collective mpi/collective.cpp
			LIBRARIES ${MPI_CXX_LIBRARIES}
			COMPILE_DEFINITIONS ${MPI_CXX_COMPILE_DEFINITIONS}
			INCLUDE_DIRECTORIES ${MPI_CXX_INCLUDE_DIRS})

		add_h5cpp_example(mpi-independent mpi/independent.cpp
			LIBRARIES ${MPI_CXX_LIBRARIES}
			COMPILE_DEFINITIONS ${MPI_CXX_COMPILE_DEFINITIONS}
			INCLUDE_DIRECTORIES ${MPI_CXX_INCLUDE_DIRS})

		add_h5cpp_example(mpi-throughput mpi/throughput.cpp
			LIBRARIES ${MPI_CXX_LIBRARIES}
			COMPILE_DEFINITIONS ${MPI_CXX_COMPILE_DEFINITIONS}
			INCLUDE_DIRECTORIES ${MPI_CXX_INCLUDE_DIRS})
	else()
		message("-- Parallel HDF5 NOT found (HDF5 built without --enable-parallel):")
		message("--   skipping mpi-collective / mpi-independent / mpi-throughput")
		message("--   mpi-file-per-rank still builds — it uses serial HDF5 per rank")
	endif()
else()
	message("-- MPI not found: skipping all mpi/* examples")
endif()

# ------------------------------------------------------------------------------
# Core examples
# ------------------------------------------------------------------------------
if( ${ARMADILLO_FOUND} )
## attributes ###
# Pick up include directories for each vendored linalg target so the h5cpp-compiler
# preprocessor pass can resolve the library headers (it parses attributes.cpp without
# the regular CMake target propagation).
get_target_property(H5CPP_EX_EIGEN3_INC    Eigen3::Eigen   INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(H5CPP_EX_BLAZE_INC     libblaze        INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(H5CPP_EX_BLITZ_INC     libblitz        INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(H5CPP_EX_DLIB_INC      libdlib         INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(H5CPP_EX_UBLAS_INC     libublas        INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(H5CPP_EX_XTENSOR_INC   libxtensor      INTERFACE_INCLUDE_DIRECTORIES)
get_target_property(H5CPP_EX_XTL_INC       xtl::xtl        INTERFACE_INCLUDE_DIRECTORIES)
add_h5cpp_example(attributes attributes/attributes.cpp
	# generated.h is committed alongside struct.h — no GENERATED line. The
	# h5cpp-compiler tool segfaults on a few of the canonical-mapping types
	# this example exercises (std::complex / std::array<char,N> appearing in
	# the same TU); the existing generated.h covers the sn::example::record_t
	# registration which is the only compiler-emitted body the example needs.
	LIBRARIES libarmadillo Eigen3::Eigen libblaze libblitz libdlib libublas libxtensor xtl::xtl
	COMPILER_INCLUDES
		${H5CPP_EX_EIGEN3_INC}
		${H5CPP_EX_BLAZE_INC}
		${H5CPP_EX_BLITZ_INC}
		${H5CPP_EX_DLIB_INC}
		${H5CPP_EX_UBLAS_INC}
		${H5CPP_EX_XTENSOR_INC}
		${H5CPP_EX_XTL_INC})
endif()

## basics ###
add_h5cpp_example(basics basics/basics.cpp)

## container ###
add_h5cpp_example(container container/container.cpp)

## detected — header-only custom containers picked up by detection idiom ###
add_h5cpp_example(detected container/detected.cpp)

## custom-pipeline — write/read pipeline opt-in surfaces ###
# Three sections: default basic_pipeline_t, h5::high_throughput DAPL flag,
# and h5::threads{N} + h5::backpressure{M} pool_pipeline_t (Phase 1.3.3).
add_h5cpp_example(custom-pipeline custom-pipeline/pipeline.cpp
	LIBRARIES Threads::Threads)

## pprint ###
# Stream-inserter demonstration for the operator<< overloads in H5Uall.hpp.
# Covers iterable containers, stack/queue/priority_queue adaptors, pairs,
# tuples, and deeply-nested compositions. No HDF5 file is opened.
add_h5cpp_example(pprint pprint/pprint.cpp)

if( ${ARMADILLO_FOUND} )
## compound ###
add_h5cpp_example(compound compound/compound.cpp
	GENERATED compound/generated.h
	COMPILER_SOURCES compound/non-pod.h
	LIBRARIES libarmadillo)
endif()

## reflection ###
# Showcases the full power of h5cpp compile-time reflection in one TU:
# tier-1 POD (H5CPP_REGISTER_STRUCT) + tier-2 VLEN (scatter/gather), with
# every supported annotation exercised.  No linalg dependency.
add_h5cpp_example(reflection reflection/reflection.cpp
	GENERATED reflection/generated.h
	COMPILER_SOURCES reflection/types.h)

## csv ###
add_h5cpp_example(csv csv/csv2hdf5.cpp
	GENERATED csv/generated.h
	LIBRARIES Threads::Threads h5cpp::csv)
if(MSVC)
    target_compile_definitions(examples-csv PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
# csv2hdf5 opens "input.csv" from the current working directory. The binary
# lands in CMAKE_BINARY_DIR (RUNTIME_OUTPUT_DIRECTORY), but the data file is in
# the source tree — stage it next to the binary so `./examples-csv` works.
add_custom_command(TARGET examples-csv POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different
		${CMAKE_CURRENT_SOURCE_DIR}/csv/input.csv
		${CMAKE_BINARY_DIR}/input.csv
	COMMENT "Staging csv/input.csv next to examples-csv binary")

## datasets ###
# Pure-STL tutorial of the dataset surface. No linalg dependency.
# Bumped to C++23 so the std::mdspan section can compile against <mdspan>;
# the mdspan path is feature-gated on __cpp_lib_mdspan in the source.
add_h5cpp_example(datasets datasets/datasets.cpp)
target_compile_features(examples-datasets PRIVATE cxx_std_23)

## datatypes ###
# Custom datatype tutorial: six sections covering strong typedef, N-bit packed
# integer, opaque bytes, naming/introspection, std::float16_t (C++23-gated),
# and H5T_ENUM. All four user-defined types live in custom_types.hpp.
# Bumped to C++23 so the std::float16_t section can compile against <stdfloat>;
# the half-float path is feature-gated on __STDCPP_FLOAT16_T__ in the source.
add_h5cpp_example(datatypes datatypes/datatypes.cpp)
target_compile_features(examples-datatypes PRIVATE cxx_std_23)

## half-float — third-party variants of the std::float16_t pattern ###
# Two backends, same h5cpp dt_t<T> specialization technique. Both vendor
# libraries are shipped under thirdparty/ and exposed as CMake targets.
# For the C++23-native std::float16_t path see examples/datatypes section 5.
add_h5cpp_example(half-float-rau     half-float/christian-rau/half-float.cpp
    LIBRARIES h5cpp::half)
add_h5cpp_example(half-float-openexr half-float/open-exr/half-float.cpp
    LIBRARIES h5cpp::imath)

## multi translation unit ###
# Demonstrates that the H5CPP_REGISTER_STRUCT bodies in `multi-tu/generated.h`
# survive being included by two TUs (the headers themselves are #pragma once,
# the macro expansions are `inline`). Requires Armadillo because tu-01 mixes a
# linalg write into the same TU as the compound `h5::create` calls.
if( ${ARMADILLO_FOUND} )
    add_executable(examples-multi-tu EXCLUDE_FROM_ALL
        "multi-tu/main.cpp" "multi-tu/tu-01.cpp" "multi-tu/tu-02.cpp")
    target_link_libraries(examples-multi-tu PRIVATE ${LIBS} libarmadillo)
    target_compile_options(examples-multi-tu PRIVATE "-w")
    set_target_properties(examples-multi-tu PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    add_dependencies(examples examples-multi-tu)
endif()

if( ${ARMADILLO_FOUND} )
## optimized ###
add_h5cpp_example(optimized optimized/optimized.cpp
	LIBRARIES libarmadillo)
endif()

if( ${Eigen3_FOUND} AND ${ARMADILLO_FOUND} )
	## packet-table ###
	# generated.h is checked-in alongside struct.h — no GENERATED line, no
	# h5cpp-compiler invocation at build time.  Update by hand or by running
	# the h5cpp compiler manually if struct.h changes.
	add_h5cpp_example(packet-table packet-table/packettable.cpp
		LIBRARIES libarmadillo Eigen3::Eigen)
	add_h5cpp_example(packet-batches packet-table/packet_batches.cpp
		LIBRARIES libarmadillo)
endif()

add_h5cpp_example(raw raw_memory/raw.cpp)
## smart-ptr ###
# Demonstrates the std::unique_ptr<T[]>, std::shared_ptr<T[]>, std::unique_ptr<T>,
# std::shared_ptr<T> integration added in h5cpp/H5Mmemory.hpp + H5Mmemory_io.hpp.
add_h5cpp_example(smart-ptr smart-ptr/smart_ptr.cpp)
add_h5cpp_example(valarray linalg/valarray.cpp)
## stl ###
# Full showdown of STL types supported by the dispatch matrix. Each family
# is its own binary so the failure surface stays narrow on bisection.
# See examples/stl/README.md for the type matrix.
add_h5cpp_example(stl-sequences    stl/sequences.cpp)
add_h5cpp_example(stl-sets         stl/sets.cpp)
add_h5cpp_example(stl-maps         stl/maps.cpp)
add_h5cpp_example(stl-strings      stl/strings.cpp)
add_h5cpp_example(stl-tuples-pairs stl/tuples_pairs.cpp)
add_h5cpp_example(stl-nested       stl/nested.cpp)
# generated.h is committed alongside struct.h — no GENERATED line.
add_h5cpp_example(stl-compound     stl/compound.cpp)
add_h5cpp_example(string string/string.cpp)
if( ${ARMADILLO_FOUND} )
	add_h5cpp_example(transform transform/transform.cpp
		LIBRARIES libarmadillo)
	endif()
	add_h5cpp_example(groups groups/groups.cpp)
add_h5cpp_example(reference reference/reference.cpp LIBRARIES libarmadillo)

## utf ###
add_h5cpp_example(utf utf/utf.cpp LIBRARIES libarmadillo)

## cout — operator<< pretty-printers from H5cout.hpp / H5Uall.hpp
add_h5cpp_example(cout cout/cout.cpp)

## mdspan — std::mdspan round-trip (C++23; inert on libstdc++ < 15)
add_h5cpp_example(mdspan mdspan/mdspan.cpp)
target_compile_features(examples-mdspan PRIVATE cxx_std_23)

## swmr — Single-Writer/Multiple-Reader (#267) ###
# LINUX-ONLY: SWMR is gated to Linux this release (matches the CMAKE_SYSTEM_NAME
# gate around the SWMR headers and the #ifndef __linux__ guard in the sources).
# Two standalone programs — run examples-swmr-write in one terminal and
# examples-swmr-read in another. Skipped on non-Linux: the symbols don't exist.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	message("-- SWMR examples added (Linux-only)")
	add_h5cpp_example(swmr-write swmr/swmr-write.cpp)
	add_h5cpp_example(swmr-read  swmr/swmr-read.cpp)
	# swmr-view uses h5::view (C++20 ranges streaming view; #if __cplusplus >= 202002L)
	add_h5cpp_example(swmr-view  swmr/swmr-view.cpp)
	target_compile_features(examples-swmr-view PRIVATE cxx_std_20)
else()
	message("-- SWMR examples skipped: Linux-only feature (#267)")
endif()

add_subdirectory(s3)
