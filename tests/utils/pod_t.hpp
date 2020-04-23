/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 *     Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#ifndef H5CPP_GUARD_WizjK
#define H5CPP_GUARD_WizjK
//template specialization of pod_t to create HDF5 COMPOUND type
template<> h5::dt_t<h5::test::pod_t> inline h5::create<h5::test::pod_t>(){

    h5::dt_t<h5::test::pod_t> ct_00{H5Tcreate(H5T_COMPOUND, sizeof (h5::test::pod_t))};
    H5Tinsert(ct_00, "a",	HOFFSET(::h5::test::pod_t,a),H5T_NATIVE_INT);
    H5Tinsert(ct_00, "b",	HOFFSET(::h5::test::pod_t,b),H5T_NATIVE_FLOAT);
    H5Tinsert(ct_00, "c",	HOFFSET(::h5::test::pod_t,c),H5T_NATIVE_DOUBLE);
    return ct_00;
};

template<> h5::dt_t<h5::test::pod_t*> inline h5::create<h5::test::pod_t*>(){
    return h5::dt_t<h5::test::pod_t*>{ H5Tvlen_create(
            h5::create<h5::test::pod_t>())};
};
template <> struct h5::name <h5::test::pod_t> {
    static constexpr char const * value = "pod_t";
};
template <> struct h5::name <h5::test::pod_t*> {
    static constexpr char const * value = "pod_t*";
};

#endif
