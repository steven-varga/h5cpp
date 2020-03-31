/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 *     Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef H5CPP_GUARD_IYkyI
#define H5CPP_GUARD_IYkyI

namespace h5{
    //template specialization of sn::example::Record to create HDF5 COMPOUND type
    template<> hid_t inline register_struct<sn::example::Record>(){
        hsize_t at_00_[] ={7};            hid_t at_00 = H5Tarray_create(H5T_NATIVE_FLOAT,1,at_00_);
        hsize_t at_01_[] ={3};            hid_t at_01 = H5Tarray_create(H5T_NATIVE_DOUBLE,1,at_01_);

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::typecheck::Record));
        H5Tinsert(ct_00, "_char",	HOFFSET(sn::typecheck::Record,_char),H5T_NATIVE_CHAR);
        H5Tinsert(ct_00, "_uchar",	HOFFSET(sn::typecheck::Record,_uchar),H5T_NATIVE_UCHAR);
        H5Tinsert(ct_00, "_short",	HOFFSET(sn::typecheck::Record,_short),H5T_NATIVE_SHORT);
        H5Tinsert(ct_00, "_ushort",	HOFFSET(sn::typecheck::Record,_ushort),H5T_NATIVE_USHORT);
        H5Tinsert(ct_00, "_int",	HOFFSET(sn::typecheck::Record,_int),H5T_NATIVE_INT);
        H5Tinsert(ct_00, "_uint",	HOFFSET(sn::typecheck::Record,_uint),H5T_NATIVE_UINT);
        H5Tinsert(ct_00, "_long",	HOFFSET(sn::typecheck::Record,_long),H5T_NATIVE_LONG);
        H5Tinsert(ct_00, "_ulong",	HOFFSET(sn::typecheck::Record,_ulong),H5T_NATIVE_ULONG);
        H5Tinsert(ct_00, "_llong",	HOFFSET(sn::typecheck::Record,_llong),H5T_NATIVE_LLONG);
        H5Tinsert(ct_00, "_ullong",	HOFFSET(sn::typecheck::Record,_ullong),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_00, "_float",	HOFFSET(sn::typecheck::Record,_float),H5T_NATIVE_FLOAT);
        H5Tinsert(ct_00, "_double",	HOFFSET(sn::typecheck::Record,_double),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "_ldouble",	HOFFSET(sn::typecheck::Record,_ldouble),H5T_NATIVE_LDOUBLE);
        H5Tinsert(ct_00, "_bool",	HOFFSET(sn::typecheck::Record,_bool),H5T_NATIVE_HBOOL);
        hsize_t at_02_[] ={4};            hid_t at_02 = H5Tarray_create(ct_00,1,at_02_);

        hid_t ct_01 = H5Tcreate(H5T_COMPOUND, sizeof (sn::other::Record));
        H5Tinsert(ct_01, "idx",	HOFFSET(sn::other::Record,idx),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_01, "aa",	HOFFSET(sn::other::Record,aa),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_01, "field_02",	HOFFSET(sn::other::Record,field_02),at_01);
        H5Tinsert(ct_01, "field_03",	HOFFSET(sn::other::Record,field_03),at_02);
        hsize_t at_03_[] ={5};            hid_t at_03 = H5Tarray_create(ct_01,1,at_03_);
        hsize_t at_04_[] ={8};            hid_t at_04 = H5Tarray_create(ct_01,1,at_04_);
        hsize_t at_05_[] ={3};            hid_t at_05 = H5Tarray_create(at_04,1,at_05_);

        hid_t ct_02 = H5Tcreate(H5T_COMPOUND, sizeof (sn::example::Record));
        H5Tinsert(ct_02, "idx",	HOFFSET(sn::example::Record,idx),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_02, "field_02",	HOFFSET(sn::example::Record,field_02),at_00);
        H5Tinsert(ct_02, "field_03",	HOFFSET(sn::example::Record,field_03),at_03);
        H5Tinsert(ct_02, "field_04",	HOFFSET(sn::example::Record,field_04),at_03);
        H5Tinsert(ct_02, "field_05",	HOFFSET(sn::example::Record,field_05),at_05);

        //closing all hid_t allocations to prevent resource leakage
        H5Tclose(at_00); H5Tclose(at_01); H5Tclose(ct_00); H5Tclose(at_02); H5Tclose(ct_01);
        H5Tclose(at_03); H5Tclose(at_04); H5Tclose(at_05); 

        //if not used with h5cpp framework, but as a standalone code generator then
        //the returned 'hid_t ct_02' must be closed: H5Tclose(ct_02);
        return ct_02;
    };
}
H5CPP_REGISTER_STRUCT(sn::example::Record);

#endif
