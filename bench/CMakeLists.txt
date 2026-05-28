# SPDX-License-Identifier: MIT
# This file is part of H5CPP.
# Copyright (c) 2025-2026 Varga Labs, Toronto, ON, Canada.

find_package(ZLIB REQUIRED)

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${HDF5_C_INCLUDE_DIRS}
)

set(LIBS
    ${HDF5_LIBRARIES}
    ZLIB::ZLIB
    libh5cpp-libdeflate
)

# ─── nanobench harness (vendored single header) ─────────────────────────────────────
set(HARNESS_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/harness/nanobench.h
)

# ─── Common bench compile settings ──────────────────────────────────────────────────
function(add_bench file_name)
    set(options)
    set(one_value_args)
    set(multi_value_args LIBRARIES)
    cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    add_executable(bench-${file_name} workloads/${file_name}.cpp ${HARNESS_SOURCES})
    target_include_directories(bench-${file_name} PRIVATE ${INCLUDE_DIRS})
    target_link_libraries(bench-${file_name} PRIVATE ${LIBS} ${ARG_LIBRARIES})
    # Intentionally not adding -Wall -Wextra: h5cpp core headers have
    # pre-existing warnings that would pollute bench output. The bench
    # code itself is simple and checked with the same rigor as tests.
    set_target_properties(bench-${file_name} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
        # Drop the `bench/` path prefix so binaries land in <build>/bench-<name>
        # instead of <build>/bench/bench-<name>. Run as `./bench-<name>`.
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endfunction()

# ─── Workload benches ───────────────────────────────────────────────────────────────
add_bench(sequential)
add_bench(filesystem_ceiling)
add_bench(containers)
add_bench(chunked_filtered)
add_bench(high_throughput)
add_bench(small_object)

# ─── Aggregate target ───────────────────────────────────────────────────────────────
add_custom_target(h5cpp-bench DEPENDS
    bench-sequential
    bench-filesystem_ceiling
    bench-containers
    bench-chunked_filtered
    bench-high_throughput
    bench-small_object
)
