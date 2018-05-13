<!---
 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->


Property Lists   {#link_property_lists}
========================================

The functions, macros, and subroutines listed here are used to manipulate property list objects in various ways, including to reset property values. With the use of property lists, HDF5 functions have been implemented and can be used in applications with many fewer parameters than would be required without property lists.

some of these examples are to present the basic idea how property lists are mapped, and can be used with H5CPP 
all objects are managed: upon leaving code block will do the right thing, may be passed to CAPI calls depending 
on implicit/explicit conversion policy defined

for brevity only some of the property list are listed here, while most of the properties are mapped to managed resource 
the complex ones are left as an exercise for you to implement -- if you ever need them.
```
 The list of descriptors: 
  			attribute    : acpl_t  
  			data transfer: dapl_t  dcpl_t  dxpl_t 
  			type         : tapl_t  tcpl_t
  			file         : fapl_t  fcpl_t         fmpl_t
  			group        : gapl_t  gcpl_t
  			link         : lapl_t  lcpl_t
  			object       :         ocrl_t                ocpl_t
  			string       :         scpl_t
```




File Creation Property Lists   {#link_fcpl_l}
========================================
```cpp
// flags := H5F_ACC_TRUNC | H5F_ACC_EXCL either to truncate or open file exclusively
// you may pass CAPI property list descriptors daisy chained with '|' operator 
auto fd = h5::create("002.h5", H5F_ACC_TRUNC, 
		h5::file_space_page_size{4096} | h5::userblock{512},  // file creation properties
		h5::fclose_degree_weak | h5::fapl_core{2048,1} );     // file access properties
```

File Access Property Lists   {#link_fapl_l}
===========================================

```cpp
h5::fapl_t fapl = h5::fclose_degree_weak | h5::fapl_core{2048,1} | h5::core_write_tracking{false,1} 
			| h5::fapl_family{H5F_FAMILY_DEFAULT,0} ;
```


Data Creation Property Lists   {#link_dcpl_l}
===============================================
```cpp
h5::dcpl_t dcpl = h5::chunk{1,4,5} | h5::deflate{4} | h5::layout_compact | h5::dont_filter_partial_chunks
		| h5::fill_value<my_struct>{STR} | h5::fill_time_never | h5::alloc_time_early 
		| h5::fletcher32 | h5::shuffle | h5::nbit;
```

Data Access Property Lists   {#link_dapl_l}
===============================================




