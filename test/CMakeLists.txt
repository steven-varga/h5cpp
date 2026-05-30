# SPDX-License-Identifier: MIT
# This file is part of H5CPP.
# Copyright (c) 2025-2026 Varga Labs, Toronto, ON, Canada.

enable_testing()
find_package(ZLIB REQUIRED)

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${HDF5_C_INCLUDE_DIRS}
)

set(LIBS
    h5cpp
    ${HDF5_LIBRARIES}
    ZLIB::ZLIB
    libdoctest
    libh5cpp-libdeflate
)

# thirdparty dependencies are added by the root CMakeLists.txt

# usage:
#   add_test_case(H5cmake)
#   add_test_case(H5Iall)
#   add_test_case(H5Tmeta LIBRARIES some_extra_target)
function(add_test_case file_name)
    set(options)
    set(one_value_args)
    set(multi_value_args LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    string(TOLOWER "${file_name}" file_name_lc)
    set(test_name test-${file_name_lc})

    add_executable(${test_name} ${file_name}.cpp)

    set_target_properties(${test_name} PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    target_include_directories(${test_name} PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(${test_name} PRIVATE ${LIBS})

    foreach(lib IN LISTS ARG_LIBRARIES)
        target_link_libraries(${test_name} PRIVATE ${lib})
    endforeach()

    add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

# smoke test — validates harness builds and runs
add_test_case(example)

# ROS3 VFD — always compiled; tests self-skip when driver is absent at runtime
add_test_case(H5FDros3)

# ROS3 network smoke test — opt-in; requires outbound S3 access
# Enable with: cmake -DH5CPP_NETWORK_TESTS=ON
# Test file: s3://rhdf5-public/h5ex_t_array.h5  (eu-central-1, public read, no auth)
if(H5CPP_NETWORK_TESTS AND H5CPP_HAVE_ROS3_VFD)
    add_test_case(H5FDros3_network)
    message(STATUS "H5CPP: network tests enabled — ROS3 S3 smoke test included")
endif()

# test content lands in issue/94
add_test_case(H5Adelete)
# attribute type coverage — issue #11
add_test_case(H5Aall)
add_test_case(H5compat)
add_test_case(H5Dgather)
add_test_case(H5Dscatter)
add_test_case(H5meta)
add_test_case(H5Rreference)
add_test_case(H5Tmeta)
add_test_case(H5Dio)
add_test_case(H5Eall)
add_test_case(H5Ialgorithm)
add_test_case(H5Pall)
add_test_case(H5Sall)
add_test_case(H5capi)
add_test_case(H5cout)
add_test_case(H5Zpipeline)
add_test_case(H5Dhighthroughput)
add_test_case(H5coverage_edges)
add_test_case(H5Dappend)
add_test_case(H5Rall)
# add_test_case(H5E_exceptions)
add_test_case(H5Dio_roundtrip)
add_test_case(H5Drank)
add_test_case(H5Tcomplex)
# std::float16_t support — issue [#224].
# Compiled with C++23 so __STDCPP_FLOAT16_T__ is visible where the compiler
# provides it (clang-17+, gcc-13+ on x86).  On older compilers the guard
# produces an empty TU and the test still builds and passes silently.
add_test_case(H5Tfloat16)
set_target_properties(test-h5tfloat16 PROPERTIES CXX_STANDARD 23 CXX_STANDARD_REQUIRED OFF)
add_test_case(H5Mvalarray)
add_test_case(H5Zdeflate)
add_test_case(H5Zshuffle)
add_test_case(H5Zfletcher32)
add_test_case(H5Zlz4)
add_test_case(H5Zzstd)
add_test_case(H5Zszip)
add_test_case(H5Zpipeline_parallel_read)
add_test_case(H5Zgorilla)
# add_test_case(H5Iall)
add_test_case(H5async)        # Phase II scaffold lifecycle + submit_and_wait tests

# ---------------------------------------------------------------------------
# C++20 streaming ranges view — issue [#181].
# Requires compiler support for std::ranges; guarded by __cplusplus check
# inside the header itself, but we build the test with CXX_STANDARD 20 so
# the full path is exercised.
# ---------------------------------------------------------------------------
if(NOT MSVC OR MSVC_VERSION GREATER_EQUAL 1930)
    add_executable(test-h5dranges H5Dranges.cpp)
    set_target_properties(test-h5dranges PROPERTIES
        CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    target_include_directories(test-h5dranges PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(test-h5dranges PRIVATE ${LIBS})
    if(MSVC)
        target_compile_options(test-h5dranges PRIVATE /Zc:__cplusplus)
    endif()
    add_test(NAME test-h5dranges COMMAND test-h5dranges)
endif()

# Example-derived I/O round-trip tests (issue/145)
add_subdirectory(examples)

# ---------------------------------------------------------------------------
# MSVC overload-resolution regression tests — issue [#163].
#
# These probes pinned and now guard against the MSVC partial-ordering bug
# where the inner h5::write at H5Dwrite.hpp:21 and the variadic ref-forwarder
# at H5Dwrite.hpp:181 were ambiguous for the (ds, sp_t, sp_t, dxpl_t, T*)
# shape, plus the related (T*, count) ambiguity between the pointer overload
# at line 70/257 and the ref overload at line 181/330.
#
# Each binary is a single isolated mode so one crash never poisons the rest.
# The bug was MSVC-specific so the probes run under WIN32 only; build cost is
# negligible.  Do not bundle modes into a single doctest binary — that would
# defeat the per-mode isolation that made bisection tractable.
# ---------------------------------------------------------------------------
add_executable(test-win_stack_probe win_stack_probe.cpp)
set_target_properties(test-win_stack_probe PROPERTIES
    CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
target_include_directories(test-win_stack_probe PRIVATE ${INCLUDE_DIRS})
# Intentionally do NOT link libdoctest — keep the call chain pristine.
target_link_libraries(test-win_stack_probe PRIVATE
    ${HDF5_LIBRARIES}
    ZLIB::ZLIB
    libh5cpp-libdeflate)
if(MSVC)
    target_link_options(test-win_stack_probe PRIVATE "/STACK:67108864")
endif()
add_test(NAME test-win_stack_probe COMMAND test-win_stack_probe)

if(WIN32)
    function(add_win_lifecycle_probe name mode)
        set(test_name test-win-lifecycle-${name})
        add_executable(${test_name} win_hdf5_lifecycle_probe.cpp)
        set_target_properties(${test_name} PROPERTIES
            CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
        target_compile_definitions(${test_name} PRIVATE H5CPP_WIN_PROBE_MODE=${mode})
        target_include_directories(${test_name} PRIVATE ${INCLUDE_DIRS})
        target_link_libraries(${test_name} PRIVATE
            ${HDF5_LIBRARIES}
            ZLIB::ZLIB
            libh5cpp-libdeflate)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    add_win_lifecycle_probe(raw-minimal 1)
    add_win_lifecycle_probe(raw-property-lists 2)
    add_win_lifecycle_probe(h5cpp-static-defaults 3)
    add_win_lifecycle_probe(h5cpp-create-only 4)
    add_win_lifecycle_probe(h5cpp-high-level-write 5)
    add_win_lifecycle_probe(h5cpp-high-level-write-exit 6)
    add_win_lifecycle_probe(h5cpp-high-level-read 7)
    add_win_lifecycle_probe(h5cpp-attribute-write 8)

    function(add_win_write_path_probe name mode)
        set(test_name test-win-write-path-${name})
        add_executable(${test_name} win_h5dwrite_path_probe.cpp)
        set_target_properties(${test_name} PROPERTIES
            CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
        target_compile_definitions(${test_name} PRIVATE H5CPP_WIN_WRITE_PROBE_MODE=${mode})
        target_include_directories(${test_name} PRIVATE ${INCLUDE_DIRS})
        target_link_libraries(${test_name} PRIVATE
            ${HDF5_LIBRARIES}
            ZLIB::ZLIB
            libh5cpp-libdeflate)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    add_win_write_path_probe(ds-ptr-count 1)
    add_win_write_path_probe(ds-object 2)
    add_win_write_path_probe(fd-ptr-count 3)
    add_win_write_path_probe(fd-object-explicit-dims 4)
    add_win_write_path_probe(fd-object-explicit-plists 5)
    add_win_write_path_probe(fd-object-existing-dataset 6)
    add_win_write_path_probe(low-level-explicit-spaces 7)
    add_win_write_path_probe(fd-object-high-throughput-dapl 8)

    # ds.dapl validity & related lifecycle probes.
    function(add_win_dapl_probe name mode)
        set(test_name test-win-dapl-${name})
        add_executable(${test_name} win_dapl_probe.cpp)
        set_target_properties(${test_name} PROPERTIES
            CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON CXX_EXTENSIONS OFF
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
        target_compile_definitions(${test_name} PRIVATE H5CPP_WIN_DAPL_PROBE_MODE=${mode})
        target_include_directories(${test_name} PRIVATE ${INCLUDE_DIRS})
        target_link_libraries(${test_name} PRIVATE
            ${HDF5_LIBRARIES}
            ZLIB::ZLIB
            libh5cpp-libdeflate)
        add_test(NAME ${test_name} COMMAND ${test_name})
    endfunction()

    add_win_dapl_probe(inspect-create          1)
    add_win_dapl_probe(inspect-open            2)
    # mode 3 (write-normalized-dapl) intentionally NOT registered — it forces
    # ds.dapl=H5P_DEFAULT to confirm what HDF5 does with an invalid handle,
    # not a real-world scenario.  Source kept for documentation.
    add_win_dapl_probe(write-after-open        4)
    add_win_dapl_probe(read-after-create       5)
    add_win_dapl_probe(default-dapl-singleton  6)
    add_win_dapl_probe(resolved-type           7)
    add_win_dapl_probe(get-dapl-capi           8)
    add_win_dapl_probe(dapl-through-move       9)
    add_win_dapl_probe(inner-write-default     10)
endif()
