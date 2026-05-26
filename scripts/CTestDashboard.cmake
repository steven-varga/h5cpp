# Off-CI CDash submission script for h5cpp.
# Invoked via: ctest -S scripts/CTestDashboard.cmake [options] -VV
# or via the wrapper: scripts/cdash [KEY=value ...]
#
# Overridable variables (pass as -DKEY=value or KEY=value to the wrapper):
#   BUILD_TYPE        Release|Debug|RelWithDebInfo  (default: Release)
#   TRACK             Experimental|Nightly          (default: Experimental)
#   JOBS              N                             (default: logical CPU count)
#   HDF5_ROOT         /path/to/hdf5                 (default: cmake auto-detect)
#   CTEST_BUILD_NAME  label                         (default: <os>-<arch>-<compiler>-<BUILD_TYPE>)
#   SUBMIT            ON|OFF                        (default: ON)

cmake_minimum_required(VERSION 3.17)

# ── paths ──────────────────────────────────────────────────────────────────
get_filename_component(CTEST_SOURCE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(CTEST_BINARY_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/build-cdash")

# ── defaults ───────────────────────────────────────────────────────────────
if(NOT DEFINED BUILD_TYPE)  set(BUILD_TYPE "Release")      endif()
if(NOT DEFINED TRACK)       set(TRACK      "Experimental") endif()
if(NOT DEFINED SUBMIT)      set(SUBMIT     ON)             endif()
if(NOT DEFINED JOBS)
  cmake_host_system_information(RESULT JOBS QUERY NUMBER_OF_LOGICAL_CORES)
endif()

# ── site = hostname ────────────────────────────────────────────────────────
cmake_host_system_information(RESULT CTEST_SITE QUERY HOSTNAME)

# ── build name: os-arch-compiler-type (auto, overridable) ─────────────────
if(NOT DEFINED CTEST_BUILD_NAME)
  cmake_host_system_information(RESULT _os   QUERY OS_NAME)
  cmake_host_system_information(RESULT _arch QUERY OS_PLATFORM)

  if(DEFINED ENV{CXX})
    get_filename_component(_compiler "$ENV{CXX}" NAME)
  else()
    set(_compiler "c++")
  endif()

  set(CTEST_BUILD_NAME "${_os}-${_arch}-${_compiler}-${BUILD_TYPE}")
endif()

# ── generator: prefer Ninja ────────────────────────────────────────────────
find_program(_ninja ninja)
if(_ninja)
  set(CTEST_CMAKE_GENERATOR "Ninja")
else()
  set(CTEST_CMAKE_GENERATOR "Unix Makefiles")
endif()
set(CTEST_BUILD_CONFIGURATION "${BUILD_TYPE}")
set(CTEST_BUILD_FLAGS          "-j${JOBS}")

# ── cmake configure options ────────────────────────────────────────────────
set(_options
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
  -DCMAKE_CXX_STANDARD=17
  -DH5CPP_BUILD_TESTS=ON
  -DH5CPP_BUILD_EXAMPLES=ON
)
if(DEFINED ENV{CC})
  list(APPEND _options "-DCMAKE_C_COMPILER=$ENV{CC}")
endif()
if(DEFINED ENV{CXX})
  list(APPEND _options "-DCMAKE_CXX_COMPILER=$ENV{CXX}")
endif()
if(DEFINED HDF5_ROOT)
  list(APPEND _options "-DHDF5_ROOT=${HDF5_ROOT}")
endif()

# ── announce ───────────────────────────────────────────────────────────────
message(STATUS "────────────────────────────────────────")
message(STATUS "h5cpp CDash submission")
message(STATUS "  site:       ${CTEST_SITE}")
message(STATUS "  build name: ${CTEST_BUILD_NAME}")
message(STATUS "  track:      ${TRACK}")
message(STATUS "  build dir:  ${CTEST_BINARY_DIRECTORY}")
message(STATUS "  jobs:       ${JOBS}")
message(STATUS "  submit:     ${SUBMIT}")
message(STATUS "────────────────────────────────────────")

# ── pipeline ───────────────────────────────────────────────────────────────
ctest_start("${TRACK}")

ctest_configure(
  BUILD   "${CTEST_BINARY_DIRECTORY}"
  OPTIONS "${_options}"
  RETURN_VALUE _rv_configure
)

ctest_build(
  BUILD "${CTEST_BINARY_DIRECTORY}"
  RETURN_VALUE _rv_build
)

ctest_test(
  BUILD          "${CTEST_BINARY_DIRECTORY}"
  PARALLEL_LEVEL "${JOBS}"
  RETURN_VALUE   _rv_test
)

if(SUBMIT)
  ctest_submit(RETURN_VALUE _rv_submit)
  if(_rv_submit)
    message(WARNING "CDash submit returned ${_rv_submit}")
  endif()
else()
  message(STATUS "SUBMIT=OFF — skipping upload")
endif()
