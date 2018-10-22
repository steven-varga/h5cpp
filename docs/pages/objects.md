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
```yacc
integral 		:= [ unsigned | signed ] [int_8 | int_16 | int_32 | int_64 | float | double ] 
vectors  		:=  *integral
rugged_arrays 	:= **integral
string 			:= **char
linalg 			:= armadillo | eigen | 
scalar 			:= integral | pod_struct | string

# not handled yet: long double, complex, specialty types
```

**scalars:** are integral types and take up a 



Support for STL                                          {#link_stl_template_types}
=========================================
currently std::vector and std::string

TODO: write detailed description of supported types, and how memory space is mapped to file space


Support for Popular Scientific Libraries                {#link_linalg_template_types}
=========================================
operations [create | read | write] has been implemented with zero-copy whenever passed by reference. In case of **direct read** operations when the requested object is assigned, the object is created only once, because of [copy elision](https://en.cppreference.com/w/cpp/language/copy_elision).
 this mechanism is useful when sketching out idea's and your focus is on the flow as opposed to performance.
```cpp
// copy-elision or return value optimization:RVO
arma::mat rvo = h5::read<arma::mat>(fd, "path_to_object");
```

For high performance operations: within loops or sub routines, choose the function prototypes that takes reference to already created objects, then update the content with partial IO call.
```cpp
	h5::ds_t ds = h5::open( ... ) 		// open dataset
	arma::mat M(n_rows,n_cols);   		// create placeholder, data-space is reserved on the heap
	h5::count_t  count{n_rows,n_cols}; 	// describe the memory region you are reading into
	h5::offset_t offset{0,0}; 			// position we reasing data from
	// high performance loop with minimal memory operations
	for( auto i: column_indices )
		h5::read(ds, M, count, offset); // count, offset and other proeprties may be speciefied in any order
```


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

Here is the chart how supported linalg systems implement acessors, memory layout

```
		data            num elements  vec   mat:rm                mat:cm                   cube
-------------------------------------------------------------------------------------------------------------------------
eigen {.data()}          {size()}          {rows():1,cols():0}    {cols():0,rows():1}     {n/a}
arma  {.memptr()}        {n_elem}                                 {n_rows:0,n_cols:1}     {n_slices:2,n_rows:0,n_cols:1}
blaze {.data()}          {n/a}             {columns():1,rows():0} {rows():0,columns():1}  {n/a}
blitz {.data()}          {size()}          {cols:1,  rows:0}                              {slices:2, cols:1,rows:0} 
itpp  {._data()}         {length()}        {cols():1,rows():0}
ublas {.data().begin()}  {n/a}             {size2():1, size1():0}
dlib  {&ref(0,0)}        {size()}          {nc():1,    nr():0}
```
