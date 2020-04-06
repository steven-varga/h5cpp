/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 *     Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef H5CPP_GUARD_WizjK
#define H5CPP_GUARD_WizjK

namespace h5 {
    //template specialization of pod_t to create HDF5 COMPOUND type
    template<> hid_t inline register_struct<::h5::test::pod_t>(){

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (::h5::test::pod_t));
        H5Tinsert(ct_00, "a",	HOFFSET(::h5::test::pod_t,a),H5T_NATIVE_INT);
        H5Tinsert(ct_00, "b",	HOFFSET(::h5::test::pod_t,b),H5T_NATIVE_FLOAT);
        H5Tinsert(ct_00, "c",	HOFFSET(::h5::test::pod_t,c),H5T_NATIVE_DOUBLE);

        //if not used with h5cpp framework, but as a standalone code generator then
        //the returned 'hid_t ct_00' must be closed: H5Tclose(ct_00);
        return ct_00;
    };
}
namespace h5::impl::detail {
    template <> struct hid_t<::h5::test::pod_t, H5Tclose,true,true, hdf5::type> : public dt_p<::h5::test::pod_t> {
        using parent = dt_p<::h5::test::pod_t>;
        using dt_p<::h5::test::pod_t>::hid_t;
        using hidtype = ::h5::test::pod_t;
        hid_t() : parent( H5Tcopy(h5::register_struct<::h5::test::pod_t>()) ) {
            hid_t id = static_cast<hid_t>( *this );
        }
    };
}
#endif
