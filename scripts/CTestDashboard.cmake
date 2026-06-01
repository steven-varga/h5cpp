# Off-CI CDash submission script for h5cpp.
# Invoked via: ctest -S scripts/CTestDashboard.cmake [options] -VV
# or via the wrapper: scripts/cdash [KEY=value ...]
#
# Overridable variables (pass as -DKEY=value or KEY=value to the wrapper):
#   BUILD_TYPE        Release|Debug|RelWithDebInfo  (default: Debug when COVERAGE=ON, else Release)
#   TRACK             Experimental|Nightly          (default: Experimental)
#   JOBS              N                             (default: SLURM CPU allocation if set, else logical CPU count)
#   HDF5_ROOT         /path/to/hdf5                 (default: cmake auto-detect)
#   HDF5_DIR          /path/to/hdf5/cmake           (default: cmake auto-detect)
#   CTEST_BUILD_NAME  label                         (default: <os>-<arch>-<compiler>-<BUILD_TYPE>[-<node>] )
#   CTEST_SITE        label                         (default: SLURM_CLUSTER_NAME if set, else hostname)
#   SUBMIT            ON|OFF                        (default: ON)
#   COVERAGE          ON|OFF                        (default: ON; forces Debug + gcov)
#   BUILD_EXAMPLES    ON|OFF                        (default: ON)
#
# Runs unattended on desktops and under SLURM (sbatch/srun) alike. Under SLURM,
# JOBS honours the job's CPU allocation (SLURM_CPUS_PER_TASK / SLURM_CPUS_ON_NODE)
# so the build never oversubscribes a shared node, the dashboard site defaults to
# the cluster name (SLURM_CLUSTER_NAME) so all nodes group together, and the build
# name is suffixed with the compute-node hostname to keep per-node runs distinct.
# Note: compute nodes often lack direct outbound HTTPS — run on a login/interactive
# node, set HTTPS_PROXY, or pass SUBMIT=OFF on the node and resubmit from a login node.

cmake_minimum_required(VERSION 3.17)

# ── paths ──────────────────────────────────────────────────────────────────
get_filename_component(CTEST_SOURCE_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(CTEST_BINARY_DIRECTORY "${CTEST_SOURCE_DIRECTORY}/build-cdash")

# ── defaults ───────────────────────────────────────────────────────────────
if(NOT DEFINED BUILD_TYPE)
  set(BUILD_TYPE "Release")
endif()
if(NOT DEFINED TRACK)
  set(TRACK "Experimental")
endif()
if(NOT DEFINED SUBMIT)
  set(SUBMIT ON)
endif()
if(NOT DEFINED COVERAGE)
  set(COVERAGE ON)
endif()
if(COVERAGE)
  # gcov line counts are only meaningful against an unoptimised, instrumented
  # build, so coverage runs force Debug regardless of any BUILD_TYPE passed.
  set(BUILD_TYPE "Debug")
endif()
if(NOT DEFINED JOBS)
  # Under SLURM, honour the job's CPU allocation so the build does not
  # oversubscribe a shared node; fall back to the machine's logical core
  # count on a desktop / login node.
  if(DEFINED ENV{SLURM_CPUS_PER_TASK})
    set(JOBS "$ENV{SLURM_CPUS_PER_TASK}")
  elseif(DEFINED ENV{SLURM_CPUS_ON_NODE})
    set(JOBS "$ENV{SLURM_CPUS_ON_NODE}")
  else()
    cmake_host_system_information(RESULT JOBS QUERY NUMBER_OF_LOGICAL_CORES)
  endif()
endif()

# ── site: cluster name under SLURM, else hostname (overridable) ─────────────
if(NOT DEFINED CTEST_SITE)
  if(DEFINED ENV{SLURM_CLUSTER_NAME})
    set(CTEST_SITE "$ENV{SLURM_CLUSTER_NAME}")
  else()
    cmake_host_system_information(RESULT CTEST_SITE QUERY HOSTNAME)
  endif()
endif()

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

  # When running under SLURM the site is the (shared) cluster name, so append
  # the compute-node hostname to keep concurrent per-node submissions distinct.
  if(DEFINED ENV{SLURM_JOB_ID})
    cmake_host_system_information(RESULT _node QUERY HOSTNAME)
    set(CTEST_BUILD_NAME "${CTEST_BUILD_NAME}-${_node}")
  endif()
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
if(NOT DEFINED BUILD_EXAMPLES)
  set(BUILD_EXAMPLES ON)
endif()

set(_options
  -DCMAKE_BUILD_TYPE=${BUILD_TYPE}
  -DCMAKE_CXX_STANDARD=17
  -DH5CPP_BUILD_TESTS=ON
  -DH5CPP_BUILD_EXAMPLES=${BUILD_EXAMPLES}
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
# HDF5_DIR (the package config dir) is the reliable discovery knob when an
# h5cc on PATH would otherwise shadow the intended install.
if(DEFINED HDF5_DIR)
  list(APPEND _options "-DHDF5_DIR=${HDF5_DIR}")
endif()

# ── coverage instrumentation (optional) ──────────────────────────────────────
if(COVERAGE)
  list(APPEND _options
    "-DCMAKE_C_FLAGS=--coverage -fprofile-update=atomic -O0 -g"
    "-DCMAKE_CXX_FLAGS=--coverage -fprofile-update=atomic -O0 -g"
    "-DCMAKE_EXE_LINKER_FLAGS=--coverage")

  # gcov tool — honour $GCOV (e.g. gcov-14 to match g++-14), else first on PATH.
  if(DEFINED ENV{GCOV})
    set(CTEST_COVERAGE_COMMAND "$ENV{GCOV}")
  else()
    find_program(CTEST_COVERAGE_COMMAND NAMES gcov)
  endif()

  # Scope the Coverage step to library headers only.  CTestCustom.cmake already
  # carries these excludes for the build tree; we set them here too so the
  # off-CI -S run is self-contained.  H5Zpipeline_pool.hpp is dead through the
  # public API (see #286) and excluded until activation is fixed.
  list(APPEND CTEST_CUSTOM_COVERAGE_EXCLUDE
    "/thirdparty/" "/test/" "/examples/" "/usr/" "/CMakeFiles/"
    "/H5Zpipeline_pool.hpp")
endif()

# ── announce ───────────────────────────────────────────────────────────────
message(STATUS "────────────────────────────────────────")
message(STATUS "h5cpp CDash submission")
message(STATUS "  site:       ${CTEST_SITE}")
message(STATUS "  build name: ${CTEST_BUILD_NAME}")
message(STATUS "  track:      ${TRACK}")
message(STATUS "  build dir:  ${CTEST_BINARY_DIRECTORY}")
message(STATUS "  jobs:       ${JOBS}")
message(STATUS "  examples:   ${BUILD_EXAMPLES}")
message(STATUS "  coverage:   ${COVERAGE}")
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

if(COVERAGE)
  ctest_coverage(
    BUILD        "${CTEST_BINARY_DIRECTORY}"
    RETURN_VALUE _rv_coverage
  )
  if(_rv_coverage)
    message(WARNING "CDash coverage step returned ${_rv_coverage}")
  endif()
endif()

if(SUBMIT)
  ctest_submit(RETURN_VALUE _rv_submit)
  if(_rv_submit)
    message(WARNING "CDash submit returned ${_rv_submit}")
  endif()
else()
  message(STATUS "SUBMIT=OFF — skipping upload")
endif()
