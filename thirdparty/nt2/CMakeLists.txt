# NT2 Numerical Template Toolbox (header-only)
# Full NT2 headers need to be added here or installed system-wide.
# The complete repository is large (~61 MB shallow clone); extract
# modules/*/include/ directories, or clone https://github.com/jtlap/nt2
# into this folder and point the include paths below accordingly.
#
# Once present, the imported target nt2::nt2 will expose the correct
# include directories to consumers.

if(NOT TARGET nt2::nt2)
    add_library(nt2::nt2 INTERFACE IMPORTED)
    # Example paths once headers are populated:
    # target_include_directories(nt2::nt2 INTERFACE
    #     ${CMAKE_CURRENT_SOURCE_DIR}/include
    #     ${CMAKE_CURRENT_SOURCE_DIR}/modules
    # )
endif()
