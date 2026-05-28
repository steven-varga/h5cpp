// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada
//
// =============================================================================
// h5cpp reflection demo — the full power, in one file
// =============================================================================
//
// What this example shows:
//
//   1. tier-1 (pure POD)
//      `sn::iot::device_t` is registered automatically via H5CPP_REGISTER_STRUCT
//      (emitted into generated.h by the h5cpp-compiler).  A `std::vector<device_t>`
//      becomes a 1-D dataset of HDF5 compound rows; round-tripping is direct.
//
//   2. tier-2 (POD + heap-indirection fields)
//      `sn::iot::event_t` carries std::string and four kinds of std::vector<T>.
//      The compiler emits `h5::scatter<event_t>` / `h5::gather<event_t>` that
//      pack/unpack each field through hvl_t.  Multiple scatter calls into the
//      same path append to the chunked dataset; gather pulls the last row.
//
//   3. Every annotation in action
//      doc, alias, version, name, name_all, ignore, chunk, compress,
//      on_missing, serialize_full — all applied in `types.h` and visible in
//      the generated.h output.
//
//   4. Type system (library dispatch — no compiler assistance needed)
//      std::tuple, std::pair, std::complex, std::map, std::set,
//      std::vector<std::string>, std::vector<std::vector<T>> — written and
//      read through h5cpp's generic access_traits_t dispatch.
//
// Run after `make examples-reflection`; the executable writes `reflection.h5`
// in the current directory.

#include "types.h"
#include <h5cpp/all>
#include "generated.h"
#include <iostream>
#include <complex>
#include <map>
#include <set>
#include <tuple>
#include <utility>
#include <vector>
#include <string>

int main() {
    h5::fd_t fd = h5::create("reflection.h5", H5F_ACC_TRUNC);

    // ─── tier-1: POD device fingerprints — direct compound dataset ──────────
    std::vector<sn::iot::device_t> devices = h5::pod<sn::iot::device_t>{} | h5::take(4);
    h5::write(fd, "/devices", devices);

    auto v_dev = h5::read<std::vector<sn::iot::device_t>>(fd, "/devices");
    std::cout << "tier-1 (POD compound):\n"
              << "    wrote " << devices.size() << " device_t rows\n"
              << "    read  " << v_dev.size()    << " device_t rows back\n"
              << "    devices[0].region_code = " << v_dev[0].region_code << "\n\n";

    // ─── tier-2: telemetry event log — append + gather ──────────────────────
    sn::iot::event_t e1;
    e1.timestamp_ns        = 1'700'000'000'000'000'000ULL;
    e1.source_id           = "rack-3.sensor-7";
    e1.connection_attempts = 5;                // [[h5::ignore]] — not persisted
    e1.payload             = {0x4d, 0xcc, 0xa1, 0xfe, 0x00};
    e1.temperatures        = {21.5f, 21.8f, 22.1f};
    e1.vibrations          = {0.001, 0.005, 0.002, 0.003};
    e1.error_codes         = {0, 12, 0};
    h5::scatter(fd, "/events", e1);

    sn::iot::event_t e2;
    e2.timestamp_ns        = 1'700'000'001'000'000'000ULL;
    e2.source_id           = "rack-3.sensor-2";
    e2.connection_attempts = 0;
    e2.payload             = {0xab, 0xcd};
    e2.temperatures        = {19.0f};
    e2.vibrations          = {};
    e2.error_codes         = {404};
    h5::scatter(fd, "/events", e2);            // appends — same scatter call

    sn::iot::event_t e_back;
    h5::gather(fd, "/events", e_back);
    std::cout << "tier-2 (VLEN compound):\n"
              << "    last event timestamp     = " << e_back.timestamp_ns << "\n"
              << "    source (renamed in file) = \"" << e_back.source_id << "\"\n"
              << "    connection_attempts      = " << e_back.connection_attempts
                                                    << "   (zero-init; [[h5::ignore]])\n"
              << "    payload.size      = " << e_back.payload.size()      << "\n"
              << "    temperatures.size = " << e_back.temperatures.size() << "\n"
              << "    vibrations.size   = " << e_back.vibrations.size()   << "\n"
              << "    error_codes.size  = " << e_back.error_codes.size()  << "\n\n";

    // ─── tier-1: name_all on pure POD ───────────────────────────────────────
    auto sensors = h5::pod<sn::iot::sensor_t>{} | h5::take(3);
    sensors[0].timestamp_ns = 1'700'000'000'000'000'001ULL;
    sensors[0].label = 42.0f;
    sensors[0].value = 98.6;
    sensors[1].timestamp_ns = 1'700'000'000'000'000'002ULL;
    sensors[1].label = 43.0f;
    sensors[1].value = 99.1;
    sensors[2].timestamp_ns = 1'700'000'000'000'000'003ULL;
    sensors[2].label = 44.0f;
    sensors[2].value = 97.8;
    h5::write(fd, "/sensors", sensors);
    auto v_sens = h5::read<std::vector<sn::iot::sensor_t>>(fd, "/sensors");
    std::cout << "tier-1 with name_all:\n"
              << "    wrote " << sensors.size() << " sensor_t rows\n"
              << "    read  " << v_sens.size() << " sensor_t rows back\n"
              << "    sensors[0].value = " << v_sens[0].value << "\n\n";

    // ─── tier-2: name_all + scatter/gather round-trip ───────────────────────
    sn::iot::session_t s1;
    s1.timestamp_ns = 1'700'000'000'000'000'004ULL;
    s1.label = "session-alpha";
    s1.internal_id = 99;                       // [[h5::ignore]]
    s1.readings = {1.1, 2.2, 3.3};
    h5::scatter(fd, "/sessions", s1);

    sn::iot::session_t s_back;
    h5::gather(fd, "/sessions", s_back);
    std::cout << "tier-2 with name_all:\n"
              << "    last session label   = \"" << s_back.label << "\"\n"
              << "    readings.size        = " << s_back.readings.size() << "\n"
              << "    internal_id          = " << s_back.internal_id
                                                << "   (zero-init; [[h5::ignore]])\n\n";

    // ─── tier-2: on_missing("ignore") — gather on absent path ───────────────
    sn::iot::probe_t probe;
    h5::gather(fd, "/probe_missing", probe);   // returns early — path absent
    std::cout << "on_missing(\"ignore\"):\n"
              << "    gather on absent path returned early\n"
              << "    probe.codes.size = " << probe.codes.size() << "\n\n";

    // ─── tier-1: nested POD compound ────────────────────────────────────────
    std::vector<sn::iot::install_t> installs = h5::pod<sn::iot::install_t>{} | h5::take(2);
    installs[0].installed_at = 1'700'000'000'000'000'005ULL;
    installs[0].device.device_id = 0xDEADBEEFULL;
    installs[0].device.firmware_version[0] = 1.0f;
    installs[0].device.firmware_version[1] = 2.0f;
    installs[0].device.firmware_version[2] = 3.0f;
    installs[0].rack_id = 7;
    installs[1].installed_at = 1'700'000'000'000'000'006ULL;
    installs[1].device.device_id = 0xCAFEBABEULL;
    installs[1].device.firmware_version[0] = 4.0f;
    installs[1].device.firmware_version[1] = 5.0f;
    installs[1].device.firmware_version[2] = 6.0f;
    installs[1].rack_id = 12;
    h5::write(fd, "/installs", installs);
    auto v_inst = h5::read<std::vector<sn::iot::install_t>>(fd, "/installs");
    std::cout << "tier-1 nested POD:\n"
              << "    wrote " << installs.size() << " install_t rows\n"
              << "    read  " << v_inst.size() << " install_t rows back\n"
              << "    installs[0].device.device_id = 0x" << std::hex
              << v_inst[0].device.device_id << std::dec << "\n\n";

    // ─── tier-1 escape hatch: serialize_full ────────────────────────────────
    sn::iot::raw_blob_t blob;
    blob.timestamp_ns = 1'700'000'000'000'000'007ULL;
    blob.label = "opaque-label";               // skipped by serialize_full
    blob.samples = {9.9, 8.8, 7.7};            // skipped by serialize_full
    blob.checksum = 0x1234;
    h5::write(fd, "/blob", blob);
    std::cout << "tier-1 serialize_full (opaque blob):\n"
              << "    wrote raw_blob_t as opaque compound (sizeof = "
              << sizeof(sn::iot::raw_blob_t) << ")\n"
              << "    non-POD fields (label, samples) skipped by compiler\n\n";

    // ─── type system: std::tuple scalar ─────────────────────────────────────
    std::tuple<int, double, char> tup{42, 3.14, 'x'};
    h5::write(fd, "/tuple", tup);
    auto tup_back = h5::read<std::tuple<int, double, char>>(fd, "/tuple");
    std::cout << "std::tuple scalar:\n"
              << "    read back = (" << std::get<0>(tup_back) << ", "
              << std::get<1>(tup_back) << ", '" << std::get<2>(tup_back) << "')\n\n";

    // ─── type system: std::vector<std::tuple<...>> ──────────────────────────
    std::vector<std::tuple<int, float>> vtups{{1, 1.1f}, {2, 2.2f}, {3, 3.3f}};
    h5::write(fd, "/vtups", vtups);
    auto vtups_back = h5::read<std::vector<std::tuple<int, float>>>(fd, "/vtups");
    std::cout << "std::vector<std::tuple<int,float>>:\n"
              << "    wrote " << vtups.size() << " tuples\n"
              << "    read  " << vtups_back.size() << " tuples back\n\n";

    // ─── type system: std::vector<std::complex> ─────────────────────────────
    std::vector<std::complex<double>> vc{{1.0, 2.0}, {3.0, 4.0}};
    h5::write(fd, "/vcomplex", vc);
    auto vc_back = h5::read<std::vector<std::complex<double>>>(fd, "/vcomplex");
    std::cout << "std::vector<std::complex<double>>:\n"
              << "    read back = [(" << vc_back[0].real() << ", " << vc_back[0].imag() << "), ...]\n\n";

    // ─── type system: std::map ──────────────────────────────────────────────
    std::map<int, double> m{{1, 1.1}, {2, 2.2}, {3, 3.3}};
    h5::write(fd, "/map", m);
    auto m_back = h5::read<std::map<int, double>>(fd, "/map");
    std::cout << "std::map<int,double>:\n"
              << "    wrote " << m.size() << " entries\n"
              << "    read  " << m_back.size() << " entries back\n"
              << "    m[2] = " << m_back[2] << "\n\n";

    // ─── type system: std::set ──────────────────────────────────────────────
    std::set<int> st{3, 1, 4, 1, 5, 9};
    h5::write(fd, "/set", st);
    auto st_back = h5::read<std::set<int>>(fd, "/set");
    std::cout << "std::set<int>:\n"
              << "    wrote " << st.size() << " unique entries\n"
              << "    read  " << st_back.size() << " unique entries back\n\n";

    // ─── type system: std::vector<std::string> ──────────────────────────────
    std::vector<std::string> vs{"alpha", "beta", "gamma"};
    h5::write(fd, "/vstr", vs);
    auto vs_back = h5::read<std::vector<std::string>>(fd, "/vstr");
    std::cout << "std::vector<std::string>:\n"
              << "    read back = [\"" << vs_back[0] << "\", \""
              << vs_back[1] << "\", \"" << vs_back[2] << "\"]\n\n";

    // ─── type system: ragged VLEN ───────────────────────────────────────────
    std::vector<std::vector<double>> ragged{{1.0}, {2.0, 3.0}, {4.0, 5.0, 6.0}};
    h5::write(fd, "/ragged", ragged);
    auto ragged_back = h5::read<std::vector<std::vector<double>>>(fd, "/ragged");
    std::cout << "std::vector<std::vector<double>> (ragged VLEN):\n"
              << "    rows = " << ragged_back.size() << "\n"
              << "    row[0].size = " << ragged_back[0].size() << "\n"
              << "    row[1].size = " << ragged_back[1].size() << "\n"
              << "    row[2].size = " << ragged_back[2].size() << "\n";
}
