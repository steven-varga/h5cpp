#pragma once
#include <string>
#include <vector>
#include <cstdint>

// =============================================================================
// h5cpp reflection demo — annotated record types
// =============================================================================
//
// Three flavours of h5cpp's compile-time reflection coexist in this header:
//
//   tier-1 (pure POD)        — H5CPP_REGISTER_STRUCT, direct HDF5 compound type.
//   tier-2 (POD + heap)      — scatter/gather specialisations that pack each
//                              std::string / std::vector<T> field into hvl_t.
//   tier-1 escape (serialize_full) — forces register_struct emission even when
//                              non-POD fields are present; non-POD members are
//                              silently skipped, producing an opaque blob.
//
// The h5cpp-compiler scans every struct, classifies it per-field by inspecting
// for heap-indirection types, and emits the right specialisation into
// `generated.h`.  Structs that no client code ever references are skipped.
//
// Every h5cpp annotation is exercised at least once:
//   [[h5::doc("...")]]              — documentation, propagated to generated.h
//   [[h5::alias("...")]]            — alternate logical name for the record
//   [[h5::version("...")]]          — schema version stamp
//   [[h5::name("...")]]             — field rename (field-level only)
//   [[h5::name_all("pre", "suf")]]  — class-level naming convention
//   [[h5::ignore]]                  — field skipped from serialisation
//   [[h5::chunk(N)]]                — chunk size for the dataset's dataspace
//   [[h5::compress("gzip", N)]]     — compression filter + level
//   [[h5::on_missing("create")]]    — behaviour when scatter/gather hits an
//                                     absent dataset (create | error | ignore)
//   [[h5::serialize_full]]          — force tier-1 emission, skip non-POD fields

namespace sn::iot {
    // Plain POD with scalars and fixed-size arrays. Maps directly to an HDF5
    // compound type without any VLEN machinery. Used as the row type of a
    // contiguous dataset of `device_t`.
    struct [[h5::doc("Persistent device fingerprint — pure POD, contiguous compound."),
            h5::alias("dev"), h5::version("1.0.0")]] device_t {
        unsigned long long              device_id;
        [[h5::name("fw")]] float        firmware_version[3];   // renamed in file
        double                          calibration[6];        // sensor calibration coefficients
        short                           region_code;
    };

    // Heap-indirection fields trigger the compiler's tier-2 path. Four different
    // VLEN element widths in one struct (uint8_t, float, double, int) — each
    // becomes its own H5Tvlen_create base type in the compound. The
    // [[h5::ignore]] field is omitted from the compound layout entirely.
    struct [[h5::doc("IoT telemetry event — VLEN payload + sensor channels."),
            h5::alias("event"), h5::version("2.1.0"), h5::chunk(256),
            h5::compress("gzip", 6), h5::on_missing("create")]] event_t {
        unsigned long long                  timestamp_ns;
        [[h5::name("source")]] std::string  source_id;             // renamed in file
        [[h5::ignore]]         int          connection_attempts;   // not persisted
        std::vector<std::uint8_t>           payload;               // VLEN bytes
        std::vector<float>                  temperatures;          // VLEN floats
        std::vector<double>                 vibrations;            // VLEN doubles
        std::vector<int>                    error_codes;           // VLEN ints
    };

    // Demonstrates class-level naming convention on a tier-1 type. Every field
    // receives the prefix/suffix unless overridden by a per-field [[h5::name]].
    struct [[h5::doc("Sensor reading with class-level naming convention."),
            h5::alias("sensor"), h5::version("1.2.0"), h5::name_all("sn_", "")]] sensor_t {
        std::uint64_t timestamp_ns;           // on-disk: sn_timestamp_ns
        [[h5::name("lbl")]] float label;      // on-disk: lbl (override)
        double value;                         // on-disk: sn_value
        [[h5::ignore]] int debug;             // omitted entirely
    };

    // Demonstrates class-level naming convention on a tier-2 type.
    struct [[h5::doc("Session data demonstrating name_all in tier-2."),
            h5::alias("session"), h5::version("3.0.0"),  h5::chunk(128), h5::compress("gzip", 4),
            h5::name_all("sess_", ""), h5::on_missing("create")]] session_t {
        std::uint64_t timestamp_ns;           // on-disk: sess_timestamp_ns
        [[h5::name("tag")]] std::string label;// on-disk: sess_tag (override)
        [[h5::ignore]] int internal_id;       // omitted
        std::vector<double> readings;         // on-disk: sess_readings
    };

    // Demonstrates the on_missing("ignore") policy: both scatter and gather
    // return early when the target dataset does not exist.
    struct [[h5::doc("Probe struct for on_missing('ignore') demonstration."),
            h5::alias("probe"), h5::version("1.0.0"), h5::chunk(64), h5::on_missing("ignore")]] probe_t {
        std::uint64_t timestamp_ns;
        std::vector<int> codes;
    };

    // Nested POD structs are valid tier-1 fields. The compiler emits a compound
    // type that embeds the already-registered child compound type.
    struct [[h5::doc("Installation record with nested device fingerprint."),
            h5::alias("install"), h5::version("1.0.0")]] install_t {
        std::uint64_t installed_at;
        [[h5::name("dev")]] device_t device;  // nested POD compound
        short rack_id;
    };

    // Forces tier-1 emission even though the struct contains non-POD fields.
    // The compiler skips std::string / std::vector members and emits a compound
    // whose size is sizeof(T) but whose only inserted members are the POD fields.
    // This produces an opaque, platform-dependent memory-layout blob.
    struct [[h5::doc("Raw opaque blob — forces tier-1 emission, non-POD fields skipped."),
            h5::alias("blob"), h5::version("0.1.0"), h5::serialize_full]] raw_blob_t {
        std::uint64_t timestamp_ns;
        std::string label;                // skipped by serialize_full
        std::vector<double> samples;      // skipped by serialize_full
        float checksum;
    };

    // Never referenced from main(); the h5cpp-compiler must detect this absence
    // of any h5::read/h5::write/h5::scatter/h5::create instantiation and skip it
    // — no entry in generated.h.
    struct unused_t {
        int  ghost;
        char marker;
    };
} // namespace sn::iot
