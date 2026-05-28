#pragma once

#include <h5cpp/all>
#include "types.h"

// doc: "Persistent device fingerprint — pure POD, contiguous compound."
// alias: "dev"
// version: "1.0.0"
namespace h5 {
    template<> hid_t inline register_struct<sn::iot::device_t>(){
        hsize_t at_00_[] ={3};            hid_t at_00 = H5Tarray_create(H5T_NATIVE_FLOAT,1,at_00_);
        hsize_t at_01_[] ={6};            hid_t at_01 = H5Tarray_create(H5T_NATIVE_DOUBLE,1,at_01_);

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::iot::device_t));
        H5Tinsert(ct_00, "device_id",	HOFFSET(sn::iot::device_t,device_id),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_00, "fw",	HOFFSET(sn::iot::device_t,firmware_version),at_00);
        H5Tinsert(ct_00, "calibration",	HOFFSET(sn::iot::device_t,calibration),at_01);
        H5Tinsert(ct_00, "region_code",	HOFFSET(sn::iot::device_t,region_code),H5T_NATIVE_SHORT);

        //closing all hid_t allocations to prevent resource leakage
        H5Tclose(at_00); H5Tclose(at_01); 

        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(sn::iot::device_t);

// doc: "Sensor reading with class-level naming convention."
// alias: "sensor"
// version: "1.2.0"
namespace h5 {
    template<> hid_t inline register_struct<sn::iot::sensor_t>(){

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::iot::sensor_t));
        H5Tinsert(ct_00, "sn_timestamp_ns",	HOFFSET(sn::iot::sensor_t,timestamp_ns),H5T_NATIVE_ULONG);
        H5Tinsert(ct_00, "lbl",	HOFFSET(sn::iot::sensor_t,label),H5T_NATIVE_FLOAT);
        H5Tinsert(ct_00, "sn_value",	HOFFSET(sn::iot::sensor_t,value),H5T_NATIVE_DOUBLE);

        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(sn::iot::sensor_t);

// doc: "Installation record with nested device fingerprint."
// alias: "install"
// version: "1.0.0"
namespace h5 {
    template<> hid_t inline register_struct<sn::iot::install_t>(){
        hsize_t at_00_[] ={3};            hid_t at_00 = H5Tarray_create(H5T_NATIVE_FLOAT,1,at_00_);
        hsize_t at_01_[] ={6};            hid_t at_01 = H5Tarray_create(H5T_NATIVE_DOUBLE,1,at_01_);

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::iot::device_t));
        H5Tinsert(ct_00, "device_id",	HOFFSET(sn::iot::device_t,device_id),H5T_NATIVE_ULLONG);
        H5Tinsert(ct_00, "fw",	HOFFSET(sn::iot::device_t,firmware_version),at_00);
        H5Tinsert(ct_00, "calibration",	HOFFSET(sn::iot::device_t,calibration),at_01);
        H5Tinsert(ct_00, "region_code",	HOFFSET(sn::iot::device_t,region_code),H5T_NATIVE_SHORT);

        hid_t ct_01 = H5Tcreate(H5T_COMPOUND, sizeof (sn::iot::install_t));
        H5Tinsert(ct_01, "installed_at",	HOFFSET(sn::iot::install_t,installed_at),H5T_NATIVE_ULONG);
        H5Tinsert(ct_01, "dev",	HOFFSET(sn::iot::install_t,device),ct_00);
        H5Tinsert(ct_01, "rack_id",	HOFFSET(sn::iot::install_t,rack_id),H5T_NATIVE_SHORT);

        //closing all hid_t allocations to prevent resource leakage
        H5Tclose(at_00); H5Tclose(at_01); H5Tclose(ct_00); 

        return ct_01;
    };
}
H5CPP_REGISTER_STRUCT(sn::iot::install_t);

// doc: "Raw opaque blob — forces tier-1 emission, non-POD fields skipped."
// alias: "blob"
// version: "0.1.0"
namespace h5 {
    template<> hid_t inline register_struct<sn::iot::raw_blob_t>(){

        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::iot::raw_blob_t));
        H5Tinsert(ct_00, "timestamp_ns",	HOFFSET(sn::iot::raw_blob_t,timestamp_ns),H5T_NATIVE_ULONG);
        H5Tinsert(ct_00, "checksum",	HOFFSET(sn::iot::raw_blob_t,checksum),H5T_NATIVE_FLOAT);

        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(sn::iot::raw_blob_t);

// doc: "IoT telemetry event — VLEN payload + sensor channels."
// alias: "event"
// version: "2.1.0"
namespace h5::generated::event_ {
    struct row_t {
        unsigned long long timestamp_ns;
        char*    source_id;
        hvl_t    payload;
        hvl_t    temperatures;
        hvl_t    vibrations;
        hvl_t    error_codes;
    };
    inline hid_t compound_type() {
        static const hid_t ct = []{
            hid_t v_source_id = H5Tcopy(H5T_C_S1);
            H5Tset_size(v_source_id, H5T_VARIABLE);
            hid_t v_payload = H5Tvlen_create(H5T_NATIVE_UCHAR);
            hid_t v_temperatures = H5Tvlen_create(H5T_NATIVE_FLOAT);
            hid_t v_vibrations = H5Tvlen_create(H5T_NATIVE_DOUBLE);
            hid_t v_error_codes = H5Tvlen_create(H5T_NATIVE_INT);
            hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(row_t));
            H5Tinsert(ct, "timestamp_ns", HOFFSET(row_t, timestamp_ns), H5T_NATIVE_ULLONG);
            H5Tinsert(ct, "source", HOFFSET(row_t, source_id), v_source_id);
            H5Tinsert(ct, "payload", HOFFSET(row_t, payload), v_payload);
            H5Tinsert(ct, "temperatures", HOFFSET(row_t, temperatures), v_temperatures);
            H5Tinsert(ct, "vibrations", HOFFSET(row_t, vibrations), v_vibrations);
            H5Tinsert(ct, "error_codes", HOFFSET(row_t, error_codes), v_error_codes);
            return ct;
        }();
        return ct;
    }
} // namespace h5::generated::event_

namespace h5 {
    template<> inline h5::ds_t scatter<sn::iot::event_t>(
        hid_t fd, const std::string& path, const sn::iot::event_t& obj) {
        using namespace ::h5::generated::event_;
        h5::ds_t ds;
        h5::mute();
        bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
        h5::unmute();
        if (!exists) {
            h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
            hsize_t chunk = 256;
            H5Pset_chunk(dcpl, 1, &chunk);
            H5Pset_deflate(dcpl, 6);
            hsize_t cur = 0;
            hsize_t max = H5S_UNLIMITED;
            hid_t space = H5Screate_simple(1, &cur, &max);
            ds = h5::createds(fd, path, compound_type(), h5::sp_t{space},
                h5::default_lcpl, dcpl, h5::default_dapl);
        } else ds = h5::open(fd, path, h5::default_dapl);
        hsize_t row = h5::detail::next_row(ds);
        hsize_t source_id_len = obj.source_id.size();
        const char* source_id_ptr = obj.source_id.c_str();
        hsize_t payload_len = obj.payload.size();
        auto* payload_ptr = obj.payload.data();
        hsize_t temperatures_len = obj.temperatures.size();
        auto* temperatures_ptr = obj.temperatures.data();
        hsize_t vibrations_len = obj.vibrations.size();
        auto* vibrations_ptr = obj.vibrations.data();
        hsize_t error_codes_len = obj.error_codes.size();
        auto* error_codes_ptr = obj.error_codes.data();
        row_t r{
            obj.timestamp_ns,
            (char*)source_id_ptr,
            hvl_t{payload_len, (void*)payload_ptr},
            hvl_t{temperatures_len, (void*)temperatures_ptr},
            hvl_t{vibrations_len, (void*)vibrations_ptr},
            hvl_t{error_codes_len, (void*)error_codes_ptr}
        };
        herr_t err = h5::detail::write_one_row(ds, compound_type(), row, &r);
        (void)err;
        return ds;
    }
} // namespace h5

namespace h5 {
    template<> inline void gather<sn::iot::event_t>(
        hid_t fd, const std::string& path, sn::iot::event_t& obj) {
        using namespace ::h5::generated::event_;
        h5::ds_t ds = h5::open(fd, path, h5::default_dapl);
        hsize_t nrows = h5::detail::next_row(ds);
        if (nrows == 0) return;
        row_t r{};
        herr_t err = h5::detail::read_one_row(ds, compound_type(), nrows - 1, &r);
        (void)err;
        obj.timestamp_ns = r.timestamp_ns;
        if (r.source_id) obj.source_id.assign(r.source_id);
        obj.payload.assign(static_cast<unsigned char*>(r.payload.p), static_cast<unsigned char*>(r.payload.p) + r.payload.len);
        obj.temperatures.assign(static_cast<float*>(r.temperatures.p), static_cast<float*>(r.temperatures.p) + r.temperatures.len);
        obj.vibrations.assign(static_cast<double*>(r.vibrations.p), static_cast<double*>(r.vibrations.p) + r.vibrations.len);
        obj.error_codes.assign(static_cast<int*>(r.error_codes.p), static_cast<int*>(r.error_codes.p) + r.error_codes.len);
        hid_t reclaim_space = H5Screate(H5S_SCALAR);
        #if H5_VERSION_GE(1,12,0)
            H5Treclaim(compound_type(), reclaim_space, H5P_DEFAULT, &r);
        #else
            H5Dvlen_reclaim(compound_type(), reclaim_space, H5P_DEFAULT, &r);
        #endif
        H5Sclose(reclaim_space);
    }
} // namespace h5

H5CPP_REGISTER_SCATTER(sn::iot::event_t);

// doc: "Session data demonstrating name_all in tier-2."
// alias: "session"
// version: "3.0.0"
namespace h5::generated::session_ {
    struct row_t {
        unsigned long timestamp_ns;
        char*    label;
        hvl_t    readings;
    };
    inline hid_t compound_type() {
        static const hid_t ct = []{
            hid_t v_label = H5Tcopy(H5T_C_S1);
            H5Tset_size(v_label, H5T_VARIABLE);
            hid_t v_readings = H5Tvlen_create(H5T_NATIVE_DOUBLE);
            hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(row_t));
            H5Tinsert(ct, "sess_timestamp_ns", HOFFSET(row_t, timestamp_ns), H5T_NATIVE_ULONG);
            H5Tinsert(ct, "tag", HOFFSET(row_t, label), v_label);
            H5Tinsert(ct, "sess_readings", HOFFSET(row_t, readings), v_readings);
            return ct;
        }();
        return ct;
    }
} // namespace h5::generated::session_

namespace h5 {
    template<> inline h5::ds_t scatter<sn::iot::session_t>(
        hid_t fd, const std::string& path, const sn::iot::session_t& obj) {
        using namespace ::h5::generated::session_;
        h5::ds_t ds;
        h5::mute();
        bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
        h5::unmute();
        if (!exists) {
            h5::dcpl_t dcpl{H5Pcreate(H5P_DATASET_CREATE)};
            hsize_t chunk = 128;
            H5Pset_chunk(dcpl, 1, &chunk);
            H5Pset_deflate(dcpl, 4);
            hsize_t cur = 0;
            hsize_t max = H5S_UNLIMITED;
            hid_t space = H5Screate_simple(1, &cur, &max);
            ds = h5::createds(fd, path, compound_type(), h5::sp_t{space},
                h5::default_lcpl, dcpl, h5::default_dapl);
        } else ds = h5::open(fd, path, h5::default_dapl);
        hsize_t row = h5::detail::next_row(ds);
        hsize_t label_len = obj.label.size();
        const char* label_ptr = obj.label.c_str();
        hsize_t readings_len = obj.readings.size();
        auto* readings_ptr = obj.readings.data();
        row_t r{
            obj.timestamp_ns,
            (char*)label_ptr,
            hvl_t{readings_len, (void*)readings_ptr}
        };
        herr_t err = h5::detail::write_one_row(ds, compound_type(), row, &r);
        (void)err;
        return ds;
    }
} // namespace h5

namespace h5 {
    template<> inline void gather<sn::iot::session_t>(
        hid_t fd, const std::string& path, sn::iot::session_t& obj) {
        using namespace ::h5::generated::session_;
        h5::ds_t ds = h5::open(fd, path, h5::default_dapl);
        hsize_t nrows = h5::detail::next_row(ds);
        if (nrows == 0) return;
        row_t r{};
        herr_t err = h5::detail::read_one_row(ds, compound_type(), nrows - 1, &r);
        (void)err;
        obj.timestamp_ns = r.timestamp_ns;
        if (r.label) obj.label.assign(r.label);
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

H5CPP_REGISTER_SCATTER(sn::iot::session_t);

// doc: "Probe struct for on_missing('ignore') demonstration."
// alias: "probe"
// version: "1.0.0"
namespace h5::generated::probe_ {
    struct row_t {
        unsigned long timestamp_ns;
        hvl_t    codes;
    };
    inline hid_t compound_type() {
        static const hid_t ct = []{
            hid_t v_codes = H5Tvlen_create(H5T_NATIVE_INT);
            hid_t ct = H5Tcreate(H5T_COMPOUND, sizeof(row_t));
            H5Tinsert(ct, "timestamp_ns", HOFFSET(row_t, timestamp_ns), H5T_NATIVE_ULONG);
            H5Tinsert(ct, "codes", HOFFSET(row_t, codes), v_codes);
            return ct;
        }();
        return ct;
    }
} // namespace h5::generated::probe_

namespace h5 {
    template<> inline h5::ds_t scatter<sn::iot::probe_t>(
        hid_t fd, const std::string& path, const sn::iot::probe_t& obj) {
        using namespace ::h5::generated::probe_;
        h5::ds_t ds;
        h5::mute();
        bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
        h5::unmute();
        if (!exists) return ds;
        ds = h5::open(fd, path, h5::default_dapl);
        hsize_t row = h5::detail::next_row(ds);
        hsize_t codes_len = obj.codes.size();
        auto* codes_ptr = obj.codes.data();
        row_t r{
            obj.timestamp_ns,
            hvl_t{codes_len, (void*)codes_ptr}
        };
        herr_t err = h5::detail::write_one_row(ds, compound_type(), row, &r);
        (void)err;
        return ds;
    }
} // namespace h5

namespace h5 {
    template<> inline void gather<sn::iot::probe_t>(
        hid_t fd, const std::string& path, sn::iot::probe_t& obj) {
        using namespace ::h5::generated::probe_;
        h5::mute();
        bool exists = H5Lexists(fd, path.c_str(), H5P_DEFAULT) > 0;
        h5::unmute();
        if (!exists) return;
        h5::ds_t ds = h5::open(fd, path, h5::default_dapl);
        hsize_t nrows = h5::detail::next_row(ds);
        if (nrows == 0) return;
        row_t r{};
        herr_t err = h5::detail::read_one_row(ds, compound_type(), nrows - 1, &r);
        (void)err;
        obj.timestamp_ns = r.timestamp_ns;
        obj.codes.assign(static_cast<int*>(r.codes.p), static_cast<int*>(r.codes.p) + r.codes.len);
        hid_t reclaim_space = H5Screate(H5S_SCALAR);
        #if H5_VERSION_GE(1,12,0)
            H5Treclaim(compound_type(), reclaim_space, H5P_DEFAULT, &r);
        #else
            H5Dvlen_reclaim(compound_type(), reclaim_space, H5P_DEFAULT, &r);
        #endif
        H5Sclose(reclaim_space);
    }
} // namespace h5

H5CPP_REGISTER_SCATTER(sn::iot::probe_t);


