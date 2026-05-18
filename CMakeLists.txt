# ___________________________________________________________________________________
# SPDX-License-Identifier: MIT
# This file is part of H5CPP.
# Copyright (c) 2025-2026 Varga Labs, Toronto, ON, Canada.
# __________________________________________________________________________________

cmake_minimum_required(VERSION 3.22.0)

# Must be set before project() — CMake's Apple toolchain reads this at project() time.
# macOS 14.0 is the minimum deployment target that exposes std::stop_token in Apple's libc++.
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "14.4" CACHE STRING "Minimum macOS deployment version")
endif()

# Derive package version from the latest git tag before project(), so CPack
# embeds it as PROJECT_VERSION rather than a stale hardcoded number.  Falls
# back to a placeholder when building from a tarball without git history.
find_package(Git QUIET)
set(H5CPP_PROJECT_VERSION "1.12.0")
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 --match "v[0-9]*"
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE H5CPP_GIT_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(H5CPP_GIT_TAG MATCHES "^v([0-9]+(\\.[0-9]+)*)$")
        set(H5CPP_PROJECT_VERSION "${CMAKE_MATCH_1}")
    endif()
endif()

project(libh5cpp-dev VERSION ${H5CPP_PROJECT_VERSION} LANGUAGES CXX C)

# ─── Standard Settings ────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(WARNING "In-source builds are discouraged.")
else()
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        OUTPUT_QUIET ERROR_QUIET
    )
endif()

# ─── Git Version Injection ────────────────────────────────────────────────────────
execute_process(COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
execute_process(COMMAND git describe --tags --abbrev=0
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

set(H5CPP_BASE_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
if(GIT_TAG)
    set(H5CPP_VERSION "${GIT_TAG}")
else()
    set(H5CPP_VERSION "${H5CPP_BASE_VERSION}-${GIT_COMMIT_HASH}")
endif()

# ─── Options ──────────────────────────────────────────────────────────────────────
option(H5CPP_BUILD_EXAMPLES "Build examples" OFF)
option(H5CPP_BUILD_TESTS    "Build tests"    OFF)
option(H5CPP_BUILD_BENCH    "Build benches"  OFF)
option(H5CPP_USE_LIBDEFLATE "Use libdeflate for H5CPP deflate/gzip filters when available" ON)
option(H5CPP_REQUIRE_LIBDEFLATE "Fail configuration if H5CPP_USE_LIBDEFLATE is ON and libdeflate is not found" OFF)
option(H5CPP_INSTALL_LIBDEFLATE "Install the resolved libdeflate header and library with H5CPP" OFF)
option(H5CPP_USE_LZ4  "Use LZ4 compression filter when available" ON)
option(H5CPP_USE_ZSTD "Use Zstd compression filter when available" ON)
option(H5CPP_USE_SZIP "Use vendored SZIP (Rice) compression filter when available" ON)

# ─── System Dependencies ──────────────────────────────────────────────────────────
include(GNUInstallDirs)

find_package(Threads REQUIRED QUIET)
find_package(Python3 COMPONENTS Interpreter QUIET)

# match hdf5 versioning x.x.x.h5cpp-version
set(H5CPP_VERSION
    ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}
)

if(WIN32)
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "C:/h5cpp" CACHE PATH "default install path" FORCE)
    endif()
elseif(UNIX)
    set(HDF5_ROOT "/usr/local" "/usr")
    find_package(PkgConfig REQUIRED QUIET)
endif()

if(APPLE)
    message(STATUS "H5CPP macOS support: CI runs on macos-13 with Homebrew HDF5 and LLVM clang-17/18")
endif()

if(ANDROID)
    message(STATUS "***WARNING: h5cpp is not fully tested on android ***")
endif()

find_package(Threads REQUIRED QUIET)

# ─── HDF5 ─────────────────────────────────────────────────────────────────────────
find_package(HDF5 REQUIRED COMPONENTS C)

# HDF5 floor is decoupled from h5cpp package version.  Previously the check
# compared HDF5_VERSION against PROJECT_VERSION, which coupled package
# versioning to HDF5 minimums by coincidence.  After #234 raised the floor
# to 1.12 and #247 made PROJECT_VERSION track the git tag, the comparison
# stopped being meaningful.  Pin the floor explicitly.
set(H5CPP_HDF5_FLOOR "1.12.0")
if(HDF5_VERSION VERSION_LESS ${H5CPP_HDF5_FLOOR})
    message(FATAL_ERROR
        "-- !!! H5CPP requires HDF5 v${H5CPP_HDF5_FLOOR} or greater (found ${HDF5_VERSION}) !!!"
    )
else()
    message(STATUS
        "H5CPP ${PROJECT_VERSION} matches with minimum required HDF5 v${HDF5_VERSION}"
    )
endif()

if(HDF5_VERSION VERSION_GREATER 1.12.0)
    message(STATUS "H5CPP KITA support is available, see webpage for details...")
endif()

# ─── ROS3 VFD detection ───────────────────────────────────────────────────────────
foreach(_hdf5_inc IN LISTS HDF5_INCLUDE_DIRS)
    foreach(_pubconf IN ITEMS "H5pubconf.h" "H5pubconf-64.h")
        if(EXISTS "${_hdf5_inc}/${_pubconf}")
            file(STRINGS "${_hdf5_inc}/${_pubconf}" _ros3_line REGEX "H5_HAVE_ROS3_VFD")
            if(_ros3_line MATCHES "#define H5_HAVE_ROS3_VFD")
                set(H5CPP_HAVE_ROS3_VFD TRUE)
                break()
            endif()
        endif()
    endforeach()
    if(H5CPP_HAVE_ROS3_VFD)
        break()
    endif()
endforeach()
if(H5CPP_HAVE_ROS3_VFD)
    message(STATUS "H5CPP: ROS3 VFD detected — S3 read-only support enabled")
else()
    message(STATUS "H5CPP: ROS3 VFD not found — S3 support disabled")
endif()

option(H5CPP_NETWORK_TESTS "Enable network-dependent tests (requires outbound S3 access)" OFF)

# ─── MPI (optional) ───────────────────────────────────────────────────────────────
find_package(MPI QUIET COMPONENTS C CXX)
if(MPI_FOUND AND HDF5_IS_PARALLEL)
    message(STATUS "MPI and PHDF5 found: Parallel H5CPP enabled")
endif()

# ─── Include Directories & Libraries ──────────────────────────────────────────────
find_package(ZLIB REQUIRED)

list(APPEND H5CPP_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${HDF5_INCLUDE_DIRS}
)

list(APPEND H5CPP_LIBS
    Threads::Threads
    ${HDF5_LIBRARIES}
    ZLIB::ZLIB
)

# ─── Thirdparty ───────────────────────────────────────────────────────────────────
add_subdirectory(thirdparty EXCLUDE_FROM_ALL)

if(H5CPP_LIBDEFLATE_FOUND)
    message(STATUS "H5CPP libdeflate support: vendored ${H5CPP_LIBDEFLATE_INCLUDE_DIR}")
elseif(H5CPP_USE_LIBDEFLATE)
    message(STATUS "H5CPP libdeflate support: not found; zlib fallback will be used")
endif()

if(H5CPP_INSTALL_LIBDEFLATE AND NOT H5CPP_LIBDEFLATE_FOUND)
    message(FATAL_ERROR "H5CPP_INSTALL_LIBDEFLATE is ON, but libdeflate was not found")
endif()

# ─── LZ4 (optional) ───────────────────────────────────────────────────────────────
if(H5CPP_USE_LZ4)
    find_library(H5CPP_LZ4_LIBRARY NAMES lz4)
    find_path(H5CPP_LZ4_INCLUDE_DIR NAMES lz4.h)
    if(H5CPP_LZ4_LIBRARY AND H5CPP_LZ4_INCLUDE_DIR)
        message(STATUS "H5CPP LZ4 support: ${H5CPP_LZ4_LIBRARY}")
    else()
        message(STATUS "H5CPP LZ4 support: not found")
    endif()
endif()

# ─── Zstd (optional) ──────────────────────────────────────────────────────────────
if(H5CPP_USE_ZSTD)
    if(H5CPP_ZSTD_FOUND)
        message(STATUS "H5CPP Zstd support: vendored ${H5CPP_ZSTD_INCLUDE_DIR}")
    else()
        message(STATUS "H5CPP Zstd support: not found")
    endif()
endif()

# ─── SZIP (optional) ──────────────────────────────────────────────────────────────
if(H5CPP_USE_SZIP)
    if(H5CPP_SZIP_FOUND)
        message(STATUS "H5CPP SZIP support: vendored ${H5CPP_SZIP_INCLUDE_DIR}")
    else()
        message(STATUS "H5CPP SZIP support: not found")
    endif()
endif()

# ─── Header-only Library Target ───────────────────────────────────────────────────
add_library(h5cpp INTERFACE)
add_library(h5cpp::h5cpp ALIAS h5cpp)

target_include_directories(h5cpp INTERFACE
    $<BUILD_INTERFACE:${H5CPP_INCLUDE_DIRS}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(h5cpp INTERFACE ${H5CPP_LIBS})
target_compile_features(h5cpp INTERFACE cxx_std_20)

# MSVC does not update __cplusplus to reflect the actual standard without this flag;
# without it the #error guard in H5Qall.hpp always fires even under /std:c++20.
if(MSVC)
    target_compile_options(h5cpp INTERFACE /Zc:__cplusplus)
endif()

if(H5CPP_HAVE_ROS3_VFD)
    target_compile_definitions(h5cpp INTERFACE H5CPP_HAVE_ROS3_VFD=1)
endif()

if(H5CPP_LIBDEFLATE_FOUND)
    target_link_libraries(h5cpp INTERFACE ${H5CPP_LIBDEFLATE_TARGET})
    target_compile_definitions(h5cpp INTERFACE H5CPP_HAS_LIBDEFLATE=1)
else()
    target_compile_definitions(h5cpp INTERFACE H5CPP_DISABLE_LIBDEFLATE=1)
endif()

if(H5CPP_USE_LZ4 AND H5CPP_LZ4_LIBRARY AND H5CPP_LZ4_INCLUDE_DIR)
    target_compile_definitions(h5cpp INTERFACE H5CPP_HAS_LZ4=1)
    target_include_directories(h5cpp INTERFACE $<BUILD_INTERFACE:${H5CPP_LZ4_INCLUDE_DIR}>)
    target_link_libraries(h5cpp INTERFACE ${H5CPP_LZ4_LIBRARY})
endif()

if(H5CPP_USE_ZSTD AND H5CPP_ZSTD_FOUND)
    target_link_libraries(h5cpp INTERFACE ${H5CPP_ZSTD_TARGET})
endif()

if(H5CPP_USE_SZIP AND H5CPP_SZIP_FOUND)
    target_link_libraries(h5cpp INTERFACE ${H5CPP_SZIP_TARGET})
endif()

# ─── Subdirectories ───────────────────────────────────────────────────────────────
if(H5CPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

enable_testing()
if(H5CPP_BUILD_TESTS AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
    add_subdirectory(test)
endif()

if(H5CPP_BUILD_BENCH AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/bench/CMakeLists.txt)
    add_subdirectory(bench)
endif()

# ─── Install Rules ────────────────────────────────────────────────────────────────
if(WIN32)
    set(INSTALL_DIR_CMAKE    "${CMAKE_INSTALL_PREFIX}/share/h5cpp/cmake")
    set(INSTALL_DIR_INCLUDE  "${CMAKE_INSTALL_PREFIX}/include")
    set(INSTALL_DIR_EXAMPLES "${CMAKE_INSTALL_PREFIX}/examples")
elseif(UNIX)
    set(INSTALL_DIR_CMAKE    "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/cmake/h5cpp")
    set(INSTALL_DIR_INCLUDE  "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}")
    set(INSTALL_DIR_EXAMPLES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp")
endif()

message(STATUS "Install directory: ${INSTALL_DIR_INCLUDE}")

# Generate single-header amalgamation during build
set(H5CPP_AMALGAMATED_HEADER ${CMAKE_BINARY_DIR}/h5cpp.hpp)
add_custom_command(
    OUTPUT ${H5CPP_AMALGAMATED_HEADER}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/amalgamate.py
            ${CMAKE_SOURCE_DIR}/h5cpp ${H5CPP_AMALGAMATED_HEADER}
    DEPENDS ${CMAKE_SOURCE_DIR}/scripts/amalgamate.py
            ${CMAKE_SOURCE_DIR}/h5cpp/core
            ${CMAKE_SOURCE_DIR}/h5cpp/io
    COMMENT "Amalgamating h5cpp headers into h5cpp.hpp"
)
add_custom_target(h5cpp-amalgamate ALL DEPENDS ${H5CPP_AMALGAMATED_HEADER})

# Install header-only library (directory + single-header)
install(DIRECTORY h5cpp DESTINATION ${INSTALL_DIR_INCLUDE})
install(FILES ${H5CPP_AMALGAMATED_HEADER} DESTINATION ${INSTALL_DIR_INCLUDE})
install(TARGETS h5cpp EXPORT h5cpp-targets)

if(H5CPP_LIBDEFLATE_FOUND)
    install(FILES "${H5CPP_LIBDEFLATE_INCLUDE_DIR}/libdeflate.h" DESTINATION "${INSTALL_DIR_INCLUDE}")
    install(TARGETS libh5cpp-libdeflate libh5cpp-libdeflate-static
        EXPORT h5cpp-targets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

if(H5CPP_ZSTD_FOUND)
    install(TARGETS libh5cpp-zstd libh5cpp-zstd-static
        EXPORT h5cpp-targets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

if(H5CPP_SZIP_FOUND)
    install(TARGETS libh5cpp-szip libh5cpp-szip-static
        EXPORT h5cpp-targets
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

# Install examples
install(DIRECTORY examples/attributes   DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/basics       DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY thirdparty/           DESTINATION "${INSTALL_DIR_EXAMPLES}/thirdparty")
install(DIRECTORY examples/compound     DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/csv          DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/datasets     DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/datatypes    DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/half-float   DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/multi-tu     DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/optimized    DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/packet-table DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/raw_memory   DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/stl          DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/string       DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/transform    DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(DIRECTORY examples/utf          DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(FILES     examples/CMakeLists.txt DESTINATION "${INSTALL_DIR_EXAMPLES}")
install(FILES     examples/Makefile       DESTINATION "${INSTALL_DIR_EXAMPLES}")

# Package config
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

# ─── pkg-config ───────────────────────────────────────────────────────────────
configure_file(cmake/h5cpp.pc.in h5cpp.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/h5cpp.pc
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

# ─── CPack ────────────────────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME                "h5cpp-dev")
set(CPACK_PACKAGE_VENDOR              "Varga Labs")
set(CPACK_PACKAGE_CONTACT             "steven.varga@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ template library for HDF5 I/O")
set(CPACK_PACKAGE_HOMEPAGE_URL        "https://h5cpp.org")
set(CPACK_PACKAGE_VERSION             "${H5CPP_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR       "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR       "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH       "${PROJECT_VERSION_PATCH}")
set(CPACK_RESOURCE_FILE_LICENSE       "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_STRIP_FILES                 OFF)

# DEB — headers land at /usr/include/h5cpp/ (ambient, no -I flag needed)
set(CPACK_PACKAGING_INSTALL_PREFIX    "/usr")
set(CPACK_DEBIAN_PACKAGE_NAME         "h5cpp-dev")
set(CPACK_DEBIAN_PACKAGE_SECTION      "libdevel")
set(CPACK_DEBIAN_PACKAGE_DEPENDS      "libhdf5-dev")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS   "h5cpp-compiler")

# RPM
set(CPACK_RPM_PACKAGE_NAME            "h5cpp-devel")
set(CPACK_RPM_PACKAGE_LICENSE         "MIT")
set(CPACK_RPM_PACKAGE_GROUP           "Development/Libraries")
set(CPACK_RPM_PACKAGE_REQUIRES        "hdf5-devel")
set(CPACK_RPM_PACKAGE_SUGGESTS        "h5cpp-compiler")

# NSIS (Windows installer)
set(CPACK_NSIS_PACKAGE_NAME           "h5cpp ${H5CPP_VERSION}")
set(CPACK_NSIS_DISPLAY_NAME           "H5CPP ${H5CPP_VERSION}")
set(CPACK_NSIS_URL_INFO_ABOUT         "https://h5cpp.org")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

# productbuild (macOS .pkg)
set(CPACK_PRODUCTBUILD_IDENTIFIER     "org.h5cpp.h5cpp")

# Override the default per-generator filename so that amd64 / arm64 / x86_64 /
# aarch64 / darwin-arm64 / windows-amd64 produce distinct names when collected
# into a single release upload directory.  Without this, multi-arch CI matrix
# builds emit identical filenames and overwrite each other on the Release page.
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-v${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
string(TOLOWER "${CPACK_PACKAGE_FILE_NAME}" CPACK_PACKAGE_FILE_NAME)

include(CPack)

# ─── Developer convenience targets ───────────────────────────────────────────
find_program(MKDOCS_EXECUTABLE mkdocs)
find_program(DOXYGEN_EXECUTABLE doxygen)

if(MKDOCS_EXECUTABLE)
    add_custom_target(docs
        COMMAND ${MKDOCS_EXECUTABLE} build
                --config-file ${CMAKE_SOURCE_DIR}/mkdocs.yml
                --site-dir ${CMAKE_BINARY_DIR}/site
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Building MkDocs documentation → build/site"
        VERBATIM
    )
    add_custom_target(docs-serve
        COMMAND ${MKDOCS_EXECUTABLE} serve --livereload
                --config-file ${CMAKE_SOURCE_DIR}/mkdocs.yml
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Serving MkDocs with live reload (Ctrl-C to stop)"
        VERBATIM
    )
endif()

if(DOXYGEN_EXECUTABLE)
    add_custom_target(docs-doxygen
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doxy
        COMMENT "Building Doxygen API reference → docs/doxygen/html"
        VERBATIM
    )
endif()
