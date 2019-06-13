#  _____________________________________________________________________________
#  Copyright (c) <2019> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2019 Toronto, On Canada
#  _____________________________________________________________________________



cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

## match hdf5 versioning x.x.x.h5cpp-version
project(h5cpp-dev VERSION 1.10.4.1 LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/"
	"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )
set(CMAKE_CXX_STANDARD 17)

if(WIN32)
	message(STATUS "***WARNING: h5cpp is not fully tested on windows, but know to work ***")
endif()
if(APPLE)
	message(STATUS "***WARNING: h5cpp is not fully tested on apple ***")
endif()

include(GNUInstallDirs)
find_package(Doxygen QUIET)

if(DOXYGEN_FOUND)
	#add_subdirectory (doxy)
endif()


add_library(h5cpp INTERFACE)
target_include_directories(h5cpp INTERFACE
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/h5cpp>
)


message( " ********* install ********** : ${CMAKE_INSTALL_PREFIX} ${CMAKE_INSTALL_INCLUDEDIR}")
# copy header only library into DESTDIR
install(DIRECTORY h5cpp DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR} )

install(DIRECTORY examples/attributes DESTINATION  ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/basics DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/before-after DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/compound DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/linalg DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/multi-tu DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/optimized DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/packet-table DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/raw_memory DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(DIRECTORY examples/transform DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )
install(FILES     examples/CMakeLists.txt DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/h5cpp )


#add_subdirectory(examples)

include("CPackConfig.cmake")
include(CPack)
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

