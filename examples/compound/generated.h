#pragma once

#include <h5cpp/all>
#include "pod.h"

namespace h5 {
    template<> hid_t inline register_struct<sn::example::record_t>(){
        hsize_t at_00_[] ={7};            hid_t at_00 = H5Tarray_create(H5T_NATIVE_FLOAT,1,at_00_);
        hsize_t at_01_[] ={3};            hid_t at_01 = H5Tarray_create(H5T_NATIVE_DOUBLE,1,at_01_);

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::typecheck::record_t));
        H5Tinsert(ct_00, "_char",	HOFFSET(sn::typecheck::record_t,_char),H5T_NATIVE_CHAR);
        H5Tinsert(ct_00, "_uchar",	HOFFSET(sn::typecheck::record_t,_uchar),H5T_NATIVE_UCHAR);
        H5Tinsert(ct_00, "_short",	HOFFSET(sn::typecheck::record_t,_short),H5T_NATIVE_SHORT);
        H5Tinsert(ct_00, "_ushort",	HOFFSET(sn::typecheck::record_t,_ushort),H5T_NATIVE_USHORT);
        H5Tinsert(ct_00, "_int",	HOFFSET(sn::typecheck::record_t,_int),H5T_NATIVE_INT);
        H5Tinsert(ct_00, "_uint",	HOFFSET(sn::typecheck::record_t,_uint),H5T_NATIVE_UINT);
        H5Tinsert(ct_00, "_long",	HOFFSET(sn::typecheck::record_t,_long),H5T_NATIVE_LONG);
        H5Tinsert(ct_00, "_ulong",	HOFFSET(sn::typecheck::record_t,_ulong),H5T_NATIVE_ULONG);
        H5Tinsert(ct_00, "_llong",	HOFFSET(sn::typecheck::record_t,_llong),H5T_NATIVE_LLONG);
        H5Tinsert(ct_00, "_ullong",	HOFFSET(sn::typecheck::record_t,_ullong),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_00, "_float",	HOFFSET(sn::typecheck::record_t,_float),H5T_NATIVE_FLOAT);
        H5Tinsert(ct_00, "_double",	HOFFSET(sn::typecheck::record_t,_double),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "_ldouble",	HOFFSET(sn::typecheck::record_t,_ldouble),H5T_NATIVE_LDOUBLE);
        H5Tinsert(ct_00, "_bool",	HOFFSET(sn::typecheck::record_t,_bool),H5T_NATIVE_HBOOL);
        hsize_t at_02_[] ={4};            hid_t at_02 = H5Tarray_create(ct_00,1,at_02_);

        hid_t ct_01 = H5Tcreate(H5T_COMPOUND, sizeof (sn::other::record_t));
        H5Tinsert(ct_01, "idx",	HOFFSET(sn::other::record_t,idx),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_01, "aa",	HOFFSET(sn::other::record_t,aa),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_01, "field_02",	HOFFSET(sn::other::record_t,field_02),at_01);
        H5Tinsert(ct_01, "field_03",	HOFFSET(sn::other::record_t,field_03),at_02);
        hsize_t at_03_[] ={5};            hid_t at_03 = H5Tarray_create(ct_01,1,at_03_);
        hsize_t at_04_[] ={8};            hid_t at_04 = H5Tarray_create(ct_01,1,at_04_);
        hsize_t at_05_[] ={3};            hid_t at_05 = H5Tarray_create(at_04,1,at_05_);

        hid_t ct_02 = H5Tcreate(H5T_COMPOUND, sizeof (sn::example::record_t));
        H5Tinsert(ct_02, "idx",	HOFFSET(sn::example::record_t,idx),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_02, "field_02",	HOFFSET(sn::example::record_t,field_02),at_00);
        H5Tinsert(ct_02, "field_03",	HOFFSET(sn::example::record_t,field_03),at_03);
        H5Tinsert(ct_02, "field_04",	HOFFSET(sn::example::record_t,field_04),at_03);
        H5Tinsert(ct_02, "field_05",	HOFFSET(sn::example::record_t,field_05),at_05);

        //closing all hid_t allocations to prevent resource leakage
        H5Tclose(at_00); H5Tclose(at_01); H5Tclose(ct_00); H5Tclose(at_02); H5Tclose(ct_01);
        H5Tclose(at_03); H5Tclose(at_04); H5Tclose(at_05); 

        return ct_02;
    };
}
H5CPP_REGISTER_STRUCT(sn::example::record_t);

namespace h5 {
    template<> hid_t inline register_struct<sn::typecheck::record_t>(){

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::typecheck::record_t));
        H5Tinsert(ct_00, "_char",	HOFFSET(sn::typecheck::record_t,_char),H5T_NATIVE_CHAR);
        H5Tinsert(ct_00, "_uchar",	HOFFSET(sn::typecheck::record_t,_uchar),H5T_NATIVE_UCHAR);
        H5Tinsert(ct_00, "_short",	HOFFSET(sn::typecheck::record_t,_short),H5T_NATIVE_SHORT);
        H5Tinsert(ct_00, "_ushort",	HOFFSET(sn::typecheck::record_t,_ushort),H5T_NATIVE_USHORT);
        H5Tinsert(ct_00, "_int",	HOFFSET(sn::typecheck::record_t,_int),H5T_NATIVE_INT);
        H5Tinsert(ct_00, "_uint",	HOFFSET(sn::typecheck::record_t,_uint),H5T_NATIVE_UINT);
        H5Tinsert(ct_00, "_long",	HOFFSET(sn::typecheck::record_t,_long),H5T_NATIVE_LONG);
        H5Tinsert(ct_00, "_ulong",	HOFFSET(sn::typecheck::record_t,_ulong),H5T_NATIVE_ULONG);
        H5Tinsert(ct_00, "_llong",	HOFFSET(sn::typecheck::record_t,_llong),H5T_NATIVE_LLONG);
        H5Tinsert(ct_00, "_ullong",	HOFFSET(sn::typecheck::record_t,_ullong),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_00, "_float",	HOFFSET(sn::typecheck::record_t,_float),H5T_NATIVE_FLOAT);
        H5Tinsert(ct_00, "_double",	HOFFSET(sn::typecheck::record_t,_double),H5T_NATIVE_DOUBLE);
        H5Tinsert(ct_00, "_ldouble",	HOFFSET(sn::typecheck::record_t,_ldouble),H5T_NATIVE_LDOUBLE);
        H5Tinsert(ct_00, "_bool",	HOFFSET(sn::typecheck::record_t,_bool),H5T_NATIVE_HBOOL);

        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(sn::typecheck::record_t);

#include "non-pod.h"

// doc: "Time-series sensor reading with variable-length fields"
namespace h5::generated::sn__sensor__timeseries_t_ {
    struct row_t {
        unsigned long long timestamp_ns;
        char*    tag;
        hvl_t    readings;
    };
    inline hid_t compound_type() {
        static const hid_t ct = []{
            hid_t v_tag = H5Tcopy(H5T_C_S1);
            H5Tset_size(v_tag, H5T_VARIABLE);
            hid_t v_readings = H5Tvlen_create(H5T_NATIVE_DOUBLE);
            hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(row_t));
            H5Tinsert(ct, "timestamp_ns", HOFFSET(row_t, timestamp_ns), H5T_NATIVE_ULLONG);
            H5Tinsert(ct, "label", HOFFSET(row_t, tag), v_tag);
            H5Tinsert(ct, "readings", HOFFSET(row_t, readings), v_readings);
            return ct;
        }();
        return ct;
    }
} // namespace h5::generated::sn__sensor__timeseries_t_

namespace h5 {
    template<> inline h5::ds_t scatter<sn::sensor::timeseries_t>(
        hid_t fd, const std::string& path, const sn::sensor::timeseries_t& obj) {
        using namespace ::h5::generated::sn__sensor__timeseries_t_;
        h5::ds_t ds;
        h5::mute();
        bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
        h5::unmute();
        if (!exists) {
            h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
            hsize_t chunk = 128;
            H5Pset_chunk(dcpl, 1, &chunk);
            H5Pset_deflate(dcpl, 6);
            hsize_t cur = 0;
            hsize_t max = H5S_UNLIMITED;
            hid_t space = H5Screate_simple(1, &cur, &max);
            ds = h5::createds(fd, path, compound_type(), h5::sp_t{space},
                h5::default_lcpl, dcpl, h5::default_dapl);
        } else ds = h5::open(fd, path, h5::default_dapl);
        hsize_t row = h5::detail::next_row(ds);
        hsize_t tag_len = obj.tag.size();
        const char* tag_ptr = obj.tag.c_str();
        hsize_t readings_len = obj.readings.size();
        auto* readings_ptr = obj.readings.data();
        row_t r{
            obj.timestamp_ns,
            (char*)tag_ptr,
            hvl_t{readings_len, (void*)readings_ptr}
        };
        herr_t err = h5::detail::write_one_row(ds, compound_type(), row, &r);
        (void)err;
        return ds;
    }
} // namespace h5

namespace h5 {
    template<> inline void gather<sn::sensor::timeseries_t>(
        hid_t fd, const std::string& path, sn::sensor::timeseries_t& obj) {
        using namespace ::h5::generated::sn__sensor__timeseries_t_;
        h5::ds_t ds = h5::open(fd, path, h5::default_dapl);
        hsize_t nrows = h5::detail::next_row(ds);
        if (nrows == 0) return;
        row_t r{};
        herr_t err = h5::detail::read_one_row(ds, compound_type(), nrows - 1, &r);
        (void)err;
        obj.timestamp_ns = r.timestamp_ns;
        if (r.tag) obj.tag.assign(r.tag);
        obj.readings.assign(static_cast<double*>(r.readings.p), static_cast<double*>(r.readings.p) + r.readings.len);
        hid_t reclaim_space = H5Screate(H5S_SCALAR);
        #if H5_VERSION_GE(1,12,0)
            H5Treclaim(compound_type(), reclaim_space, H5P_DEFAULT, &r);
        #else
            H5Dvlen_reclaim(compound_type(), reclaim_space, H5P_DEFAULT, &r);
        #endif
        H5Sclose(reclaim_space);
    }
} // namespace h5

H5CPP_REGISTER_SCATTER(sn::sensor::timeseries_t);


