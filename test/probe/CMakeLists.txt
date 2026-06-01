# SPDX-License-Identifier: MIT
# This file is part of H5CPP.
# Copyright (c) 2025-2026 Varga Labs, Toronto, ON, Canada.
#
# FAPL/DAPL/fileno behaviour probes — issue #286 acceptance criteria.
#
# These are plain C binaries (pure HDF5 C-API), NOT doctest / h5cpp tests.
# They exist to document HDF5 behaviour that drives the registry design.
# Each probe is wired as a ctest target so `ctest` can run them and record
# the output for the CDash dashboard.
#
# Key findings (reference: exp-w-collector/probe/):
#   fapl_embed_probe  — H5Fget_access_plist strips H5Pinsert2 properties
#                       (justifies the fileno-keyed registry instead of
#                       retrieving the pool from the file's FAPL).
#   dapl_dcpl_probe   — DAPL/DCPL custom property survival through open/close
#                       (informs which property list can carry per-dataset
#                       state; DAPL survives same-handle, not reopen).
#   fileno_probe      — H5Fget_fileno is stable across two opens of the same
#                       physical file in one process (validates the registry key).

enable_testing()

set(PROBE_INCLUDE_DIRS ${HDF5_C_INCLUDE_DIRS})
set(PROBE_LINK_LIBS    ${HDF5_LIBRARIES})

# Helper function: build a probe and register it as a ctest.
function(add_probe name)
    set(exe_name probe-${name})
    add_executable(${exe_name} ${name}.c)
    set_target_properties(${exe_name} PROPERTIES
        C_STANDARD 11 C_STANDARD_REQUIRED ON
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    target_include_directories(${exe_name} PRIVATE ${PROBE_INCLUDE_DIRS})
    target_link_libraries(${exe_name} PRIVATE ${PROBE_LINK_LIBS})
    add_test(NAME ${exe_name} COMMAND ${exe_name})
    # Probes are documentation/diagnostic, not pass/fail gates; label them so
    # `ctest -L probe` runs only this subset.
    set_tests_properties(${exe_name} PROPERTIES LABELS "probe;#286")
endfunction()

add_probe(fapl_embed_probe)
add_probe(dapl_dcpl_probe)
add_probe(fileno_probe)
