/*
 * Copyright (c) 2018 - 2021 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef H5CPP_TCONVERSION_HPP
#define H5CPP_TCONVERSION_HPP

#include <hdf5.h>
#include <iostream>
#include "H5Tall.hpp"
#include <vector>
#include <utility>

namespace h5::impl{
    template<class mem_t, class file_t = undefined_t, class element_t=typename meta::decay<mem_t>::type>
    struct convert_t {
        convert_t() = default;
        convert_t(const h5::dt_t<mem_t>& mem_type, const h5::dt_t<file_t>& file_type, 
            const h5::count_t& count, const h5::dxpl_t& dxpl, const mem_t* ptr) {
            std::cout <<"????????T mem_t\n";
            this->ptr = +ptr;
            this->count = static_cast<hsize_t>(count) ;
            set_space();
        }

        void set_space(){
            // we're transfering the same amount of data from memory as there is selected within file
			this->space = h5::create_simple(this->count);
			h5::select_all( space );
        }

        h5::dt_t<mem_t> type;
        h5::sp_t space;
        hsize_t count; 
        const void* ptr;        
    };

    template<class T, int N>
    struct convert_t <T[N], char*> {
        convert_t(const h5::dt_t<char[N]>& mem_type, const h5::dt_t<file_t>& file_type, 
            const h5::count_t& count, const h5::dxpl_t& dxpl, const T* ptr ) {
                std::cout <<"!!!!!!1[ char: " << N <<"] !!!\n";
       
        }
    };   
    /*
       template<class T, int N>
    struct convert_t <const char*[N]> : public convert_t<const char**>{
        convert_t(const h5::dt_t<char[N]>& mem_type, const h5::dt_t<file_t>& file_type, 
            const h5::count_t& count, const h5::dxpl_t& dxpl, const char* const(*ptr)[N] ) {
                std::cout <<"!!!!!!1[ char: " << N <<"] !!!\n";
       
        }
    };
    */
    // ARRAYS[N][M][..]
    template<class T, int N>
    struct convert_t <T[N]> : public convert_t<T*>{
        using convert_t<T*>::set_space;

        convert_t(const h5::dt_t<T[N]>& mem_type, const h5::dt_t<file_t>& file_type, 
            const h5::count_t& count, const h5::dxpl_t& dxpl, const T(*ptr)[N] ) {
                std::cout <<"-------" << N <<"] !!!\n";
            switch(H5Tget_class(file_type)){
                case H5T_INTEGER:
                [[fallthrough]];
                case H5T_FLOAT:  // T[] -> S hypercube 
                    using element_t = typename std::remove_all_extents<T>::type;
                    this->type = h5::create<element_t>();
                    this->count = sizeof(T[N])/sizeof(element_t);
                    break;
                case H5T_ARRAY: // T[] -> T[]
                    this->type = h5::create<T[N]>();
                    this->count = 1; // type contains the size, object count is `1`
                    break;                    
                case H5T_COMPOUND:
                case H5T_OPAQUE:
                case H5T_ENUM:
                [[fallthrough]];
                case H5T_VLEN:
                    throw(h5::error::conversion::any("not yet supported..."));
                default: break;
            }

            set_space();
            this->ptr = +ptr;
        }
    }; 
    template<int N>
    struct convert_t <char16_t[N]> : public convert_t<char16_t, char16_t[N]>{
        using convert_t<char16_t, char16_t[N]>::set_space;
        convert_t(const h5::dt_t<char16_t[N]>& mem_type, const h5::dt_t<file_t>& file_type, 
            const h5::count_t& count, const h5::dxpl_t& dxpl, const char16_t (*ptr)[N] ) {
                std::cout <<"!!!!!!!!!![ " << N <<"] !!!\n";
            this->type = h5::create<std::string>();
            hsize_t n_elements = static_cast<hsize_t>(count)  + 2;
            this->ptr = +ptr;
            set_space();
        }
    };    
}


namespace {

}
#endif
