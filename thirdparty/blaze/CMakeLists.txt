#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

set(name blaze)

set(libname lib${name})
project(${libname} LANGUAGES CXX)

set(current_version_dir "v3.8")
add_library(${libname} INTERFACE)
add_library(blaze::blaze ALIAS ${libname})
target_include_directories(${libname} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir})

# Disable Blaze's SIMD row padding so blaze::DynamicMatrix<T>::data() returns a
# truly contiguous (rows × columns) buffer.  The h5cpp mapper's `contiguous`
# access path assumes data() points at packed storage; with padding enabled
# blaze inserts unused elements at the end of each row for alignment, which
# corrupts round-trips for matrices whose column count is not a multiple of
# the SIMD width.  Officially supported Blaze switch — see
# blaze/system/Optimizations.h.
target_compile_definitions(${libname} INTERFACE BLAZE_USE_PADDING=0)
