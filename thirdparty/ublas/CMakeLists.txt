#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

set(name ublas)

set(libname lib${name})
project(${libname} LANGUAGES CXX)

set(current_version_dir "v1.89.0")
add_library(${libname} INTERFACE)
# The vendored tree is a bcp-extracted, header-only closure laid out under a
# `boost/` prefix, so <boost/numeric/ublas/...> resolves entirely from here —
# no system Boost, no compiled libs, no BLAS/LAPACK. uBLAS ships its own (slow)
# fallbacks; BLAS bindings are a separate opt-in we do not use.
target_include_directories(${libname} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/${current_version_dir})
