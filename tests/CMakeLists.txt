# check if the correct version of hdf5 available
set(H5CPP_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})
set(HDF5_FIND_DEBUG FALSE)
include(FindHDF5)
if(HDF5_VERSION VERSION_LESS ${H5CPP_VERSION})
	message(FATAL_ERROR "H5CPP examples require of HDF5 v${H5CPP_VERSION} or greater!!!")
else()
	message("-- H5CPP ${PROJECT_VERSION} matches with minimum required HDF5 v${HDF5_VERSION}")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/"
	"${CMAKE_SOURCE_DIR}/cmake/eigen3"  "${CMAKE_SOURCE_DIR}/cmake/dlib" )
set(CMAKE_CXX_STANDARD 17)

function(h5cpp_test target)
    add_executable(${target} ${ARGN})
    target_include_directories(${target}
        PRIVATE
            ${HDF5_C_INCLUDE_DIRS}
    )
    target_link_libraries(${target}
        PRIVATE
            h5cpp

            profiler
            glog
            gtest
            gtest_main
            boost_date_time
            ${HDF5_C_LIBRARIES}
            ${HDF5_C_HL_LIBRARIES}
            z
            dl
            m
            pthread
    )

    add_test(NAME ${target} COMMAND ${target} --gtest_color=auto --gtest_print_time=0)
endfunction()

h5cpp_test(test-stl         stl.cpp         event_listener.hpp abstract.h)
h5cpp_test(test-integral    integral.cpp    event_listener.hpp abstract.h)
h5cpp_test(test-pipeline    pipeline.cpp    event_listener.hpp abstract.h)
h5cpp_test(test-armadillo   armadillo.cpp   event_listener.hpp abstract.h)
h5cpp_test(test-packettable packettable.cpp event_listener.hpp struct.h)
