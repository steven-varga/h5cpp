/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef  H5CPP_DGATHER_HPP 
#define  H5CPP_DGATHER_HPP

#include <vector>
#include <string>
#include <iostream> 
#include <string.h>

namespace h5 {

    /** @brief gathers memory regions into a single set of pointers
     * Objects may be classified by whether the content resides in contiguous memory location, making convenient
     * to IO operation: only single call is needed; or scattered and we need a reliable mechanism to collect the 
     * content. <br/>
     * `h5::gather` is a template mechanism to facilitate the latter process by finding and returning a set of 
     * `element_t` type pointers to the actual content of an object.  
     * @param ref arbitrary object with non-contiguous content
     * @param ptr element type pointer with the correct size respect to `ref` object
     * @return ptr the same `element_t` pointer passed to the call 
     * @tparam T C++ type of dataset being written into HDF5 container
     */ 
    template <class T, class E> inline
    const E** gather(const T& ref, std::vector<const E*>& ptrs) {
        std::cout <<"< +++++++++ >\n";

        return ptrs.data();
    }
    /** @brief gathers memory regions into a single set of pointers
     * Objects may be classified by whether the content resides in contiguous memory location, making convenient
     * to IO operation: only single call is needed; or scattered and we need a reliable mechanism to collect the 
     * content. <br/>
     * `h5::gather` is a template mechanism to facilitate the latter process by finding and returning a set of 
     * `element_t` type pointers to the actual content of an object.  
     * @param ref arbitrary object with non-contiguous content
     * @param ptr element type pointer with the correct size respect to `ref` object
     * @return ptr the same `element_t` pointer passed to the call 
     * @tparam T C++ type of dataset being written into HDF5 container
     */ 
    template <> inline
    const char** gather<std::vector<std::string>>( const std::vector<std::string>& ref, std::vector<const char*>& ptrs){
        for(auto& element: ref) 
           ptrs.push_back(element.data());
        return ptrs.data();
    } 
}
#endif