/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 *     Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef H5CPP_GUARD_zcGEu
#define H5CPP_GUARD_zcGEu

namespace h5{
    //template specialization of s1_t to create HDF5 COMPOUND type
    template<> hid_t inline register_struct<s1_t>(){

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (s1_t));
        H5Tinsert(ct_00, "a",	HOFFSET(s1_t,a),H5T_NATIVE_INT);
        H5Tinsert(ct_00, "b",	HOFFSET(s1_t,b),H5T_NATIVE_FLOAT);
        H5Tinsert(ct_00, "c",	HOFFSET(s1_t,c),H5T_NATIVE_DOUBLE);

        //if not used with h5cpp framework, but as a standalone code generator then
        //the returned 'hid_t ct_00' must be closed: H5Tclose(ct_00);
        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(s1_t);

namespace h5{
    //template specialization of s2_t to create HDF5 COMPOUND type
    template<> hid_t inline register_struct<s2_t>(){

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (s2_t));
        H5Tinsert(ct_00, "c",	HOFFSET(s2_t,c),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "a",	HOFFSET(s2_t,a),H5T_NATIVE_INT);

        //if not used with h5cpp framework, but as a standalone code generator then
        //the returned 'hid_t ct_00' must be closed: H5Tclose(ct_00);
        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(s2_t);

#endif
