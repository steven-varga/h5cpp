<!---
 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->


C++ API   {#link_api_templ}
================================


HDF5 handlers and C++ RAII   {#link_raii_types}
================================================
All types are wrapped into `h5::impl::hid_t<T>` template then 
```yacc
T := [ file_handles | property_list ]
file_handles   := [ fd_t | ds_t | att_t | err_t | grp_t | id_t | obj_t ]
property_lists := [ file | group | link | dataset | type | object | attrib | access ]

#           create   access  transfer  copy 
file    := [fcpl_t | fapl_t                    ] 
group   := [gcpl_t | gapl_t                    ]
link    := [lcpl_t | lapl_t                    ]
dataset := [dcpl_t | dapl_t | dtpl_t           ]
type    := [         tapl_t                    ]
object  := [ocpl_t                   | ocpyl_t ]
attrib  := [acpl_t 
```

C/C++ type map to HDF5 file space  						{#link_base_template_types}
==================================

TODO: write detailed description of supported types, and how memory space is mapped to file space

Support for STL                                          {#link_stl_template_types}
=========================================
currently std::vector and std::string

TODO: write detailed description of supported types, and how memory space is mapped to file space


Support for Popular Scientific Libraries                {#link_linalg_template_types}
=========================================

TODO: write detailed description of supported types, and how memory space is mapped to file space


```yacc
T := ([unsigned] ( int8_t | int16_t | int32_t | int64_t )) | ( float | double  )
S := T | c/c++ struct | std::string
ref := std::vector<S> 
	| arma::Row<T> | arma::Col<T> | arma::Mat<T> | arma::Cube<T> 
	| Eigen::Matrix<T,Dynamic,Dynamic> | Eigen::Matrix<T,Dynamic,1> | Eigen::Matrix<T,1,Dynamic>
	| Eigen::Array<T,Dynamic,Dynamic>  | Eigen::Array<T,Dynamic,1>  | Eigen::Array<T,1,Dynamic>
	| blaze::DynamicVector<T,rowVector> |  blaze::DynamicVector<T,colVector>
	| blaze::DynamicVector<T,blaze::rowVector> |  blaze::DynamicVector<T,blaze::colVector>
	| blaze::DynamicMatrix<T,blaze::rowMajor>  |  blaze::DynamicMatrix<T,blaze::colMajor>
	| itpp::Mat<T> | itpp::Vec<T>
	| blitz::Array<T,1> | blitz::Array<T,2> | blitz::Array<T,3>
	| dlib::Matrix<T>   | dlib::Vector<T,1> 
	| ublas::matrix<T>  | ublas::vector<T>
ptr 	:= T* 
accept 	:= ref | ptr 
```

