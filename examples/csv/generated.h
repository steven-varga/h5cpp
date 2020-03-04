/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 *     Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef H5CPP_GUARD_oVQmQ
#define H5CPP_GUARD_oVQmQ

namespace h5{
    //template specialization of input_t to create HDF5 COMPOUND type
    template<> hid_t inline register_struct<input_t>(){
        hsize_t at_00_[] ={20};            hid_t at_00 = H5Tarray_create(H5T_NATIVE_CHAR,1,at_00_);

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (input_t));
        H5Tinsert(ct_00, "MasterRecordNumber",	HOFFSET(input_t,MasterRecordNumber),H5T_NATIVE_LONG);
        H5Tinsert(ct_00, "Hour",	HOFFSET(input_t,Hour),H5T_NATIVE_UINT);
        H5Tinsert(ct_00, "Latitude",	HOFFSET(input_t,Latitude),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "Longitude",	HOFFSET(input_t,Longitude),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "ReportedLocation",	HOFFSET(input_t,ReportedLocation),at_00);

        //closing all hid_t allocations to prevent resource leakage
        H5Tclose(at_00); 

        //if not used with h5cpp framework, but as a standalone code generator then
        //the returned 'hid_t ct_00' must be closed: H5Tclose(ct_00);
        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(input_t);

#endif
