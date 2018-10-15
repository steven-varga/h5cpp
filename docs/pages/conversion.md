<!---
 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->


Conversion between CAPI and C++   {#link_conversion_policy}
============================================================
TODO: port article from sandbox on implicit/explicit conversion


RAII   {#link_raii_idiom}
============================================================

There are c++ mapping for  `hid_t` id-s which reference objects with `std::shared_ptr` type of behaviour with HDF5 CAPI internal reference
counting. For further details see [H5inc_ref][1], [H5dec_ref][2] and [H5get_ref][3]. The internal representation of these objects is binary compatible of the CAPI hid_t and interchangeable depending on the conversion policy:
	`H5_some_function( static_cast<hid_t>( h5::hid_t id ), ...   )`
Direct initialization `h5::ds_t{ some_hid }` bypasses reference counting, and is intended to for use case where you have to take ownership
of a CAPI hid_t object reference. This is equivalent behaviour to `std::shared_ptr`, when object destroyed reference count is decreased.
```cpp

{
	h5::ds_t ds = h5::open( ... ); 
}
```


[1]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5I.html#Identify-IncRef
[2]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5I.html#Identify-DecRef
[3]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5I.html#Identify-GetRef

