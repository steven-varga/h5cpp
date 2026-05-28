/*
 * Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <armadillo>
#include <h5cpp/all>

#include <cmath>
#include <iostream>
#include <string>

// =========================================================================
// h5cpp data-transform & type-conversion-callback demo.
//
// HDF5 carries two orthogonal in-flight numeric features on the DXPL
// (data transfer property list):
//
//   h5::data_transform{"expr"}   element-wise linear expression in x,
//                                applied by HDF5 during H5Dread/H5Dwrite.
//                                Operators: + - * /  and parentheses.
//                                Same expression, mirror semantics:
//                                   - on write: stored = expr(memory)
//                                   - on read:  memory = expr(stored)
//                                Spec: see H5Pset_data_transform docs.
//
//   h5::type_conv_cb{ {fn, ud} } callback HDF5 invokes when an implicit
//                                type conversion would lose information
//                                (overflow, underflow, truncate, NaN/INF).
//                                The callback chooses one of:
//                                   H5T_CONV_ABORT      → propagate error
//                                   H5T_CONV_HANDLED    → continue, value
//                                                          left as written
//                                                          by the callback
//                                   H5T_CONV_UNHANDLED  → HDF5 default
//
// Both knobs pass through the dxpl, so they can be combined with any
// other DXPL option (chunk cache size, MPI collective mode, etc.) via the
// same | composition the rest of h5cpp uses.
// =========================================================================

namespace {
    int errors = 0;

    template <class A, class B>
    void check(const char* tag, const A& expected, const B& got, double tol = 1e-12) {
        const double err = std::abs(static_cast<double>(expected - got));
        const bool ok = (err <= tol);
        if (!ok) ++errors;
        std::cout << (ok ? "✔ ok      " : "✘ failed  ")
                  << tag << "  (expected=" << expected
                  << "  got=" << got << ")\n";
    }

    // Type-conversion exception callback. HDF5 calls this once per element
    // whose conversion would lose information; here we clamp instead of
    // abort. dst_buf is the in-memory destination element (sized to the
    // dst_id native type) — we cast and overwrite, then return HANDLED.
    H5T_conv_ret_t clamp_to_int16(H5T_conv_except_t except_type,
                                  hid_t /*src_id*/, hid_t /*dst_id*/,
                                  void* /*src_buf*/, void* dst_buf,
                                  void* /*op_data*/) {
        auto* dst = static_cast<std::int16_t*>(dst_buf);
        switch (except_type) {
            case H5T_CONV_EXCEPT_RANGE_HI:  *dst = std::numeric_limits<std::int16_t>::max(); return H5T_CONV_HANDLED;
            case H5T_CONV_EXCEPT_RANGE_LOW: *dst = std::numeric_limits<std::int16_t>::min(); return H5T_CONV_HANDLED;
            case H5T_CONV_EXCEPT_TRUNCATE:  return H5T_CONV_UNHANDLED;  // let HDF5 do its default truncation
            default:                        return H5T_CONV_UNHANDLED;
        }
    }
}

int main() {
    h5::fd_t fd = h5::create("transform.h5", H5F_ACC_TRUNC);

    // ---------------------------------------------------------------------
    // 1. Write-side transform.
    //    Memory: 1.0 everywhere.  Expression: 2*x + 5.
    //    On disk: every element should be 7.0.
    // ---------------------------------------------------------------------
    {
        arma::mat M(4, 7); M.ones();           // memory = 1.0
        h5::write(fd, "/scale_offset/write", M,
                  h5::data_transform{"2*x + 5"});

        // Read with no transform — we should see the on-disk values.
        auto back = h5::read<arma::mat>(fd, "/scale_offset/write");
        check("write transform 2*x+5 stored as 7", 7.0, back(0, 0));
    }

    // ---------------------------------------------------------------------
    // 2. Read-side transform.
    //    Apply x/2 - 1 on read; on-disk 7 → memory 7/2 - 1 = 2.5.
    // ---------------------------------------------------------------------
    {
        auto m = h5::read<arma::mat>(fd, "/scale_offset/write",
                                     h5::data_transform{"x/2 - 1"});
        check("read transform x/2-1 of stored 7", 2.5, m(0, 0));
    }

    // ---------------------------------------------------------------------
    // 3. Round-trip identity via composed write + read transforms.
    //    Pre-shift memory M=10. Write with `(x-10)*0` would zero everything,
    //    so we use an actual invertible pair: write x*3+2, read (x-2)/3.
    // ---------------------------------------------------------------------
    {
        arma::mat M(3, 3); M.fill(10.0);
        h5::write(fd, "/roundtrip/encoded", M,
                  h5::data_transform{"x*3 + 2"});             // disk = 32
        auto decoded = h5::read<arma::mat>(fd, "/roundtrip/encoded",
                                          h5::data_transform{"(x-2)/3"}); // memory = 10
        check("round-trip via (3x+2 write) ∘ ((x-2)/3 read) → identity",
              10.0, decoded(0, 0));
    }

    // ---------------------------------------------------------------------
    // 4. Unit conversion idiom — Celsius ↔ Kelvin on the IO boundary.
    //    Application code stays in Celsius; on-disk file is in Kelvin.
    // ---------------------------------------------------------------------
    {
        arma::mat celsius(2, 3);
        celsius << 0.0  << 25.0 << 100.0 << arma::endr
                << -40.0 << 37.0 << -273.15 << arma::endr;
        h5::write(fd, "/temperature/kelvin", celsius,
                  h5::data_transform{"x + 273.15"});

        // Verify the on-disk values are in Kelvin (no read transform).
        auto kelvin = h5::read<arma::mat>(fd, "/temperature/kelvin");
        check("Celsius 0 → Kelvin 273.15 on disk", 273.15,    kelvin(0, 0));
        check("Celsius 100 → Kelvin 373.15 on disk", 373.15,  kelvin(0, 2));
        check("Celsius -273.15 → Kelvin 0 on disk", 0.0,      kelvin(1, 2));

        // Read back into Celsius with the inverse transform.
        auto round_trip = h5::read<arma::mat>(fd, "/temperature/kelvin",
                                              h5::data_transform{"x - 273.15"});
        check("Kelvin → Celsius round-trip",  25.0, round_trip(0, 1));
    }

    // ---------------------------------------------------------------------
    // 5. Type-conversion callback — narrow double-on-disk to int16 in mem,
    //    callback clamps the overflow at int16's positive/negative limits.
    //    Without the callback, HDF5 would raise an overflow error.
    // ---------------------------------------------------------------------
    {
        arma::mat wide(1, 4);
        wide << 0.0 << 100.0 << 50'000.0 << -50'000.0 << arma::endr;
        h5::write(fd, "/narrow/source", wide);

        // Read as int16 — values 50000 / -50000 overflow int16 (±32767).
        // Without the callback this throws; with it, we clamp.
        auto narrowed = h5::read<arma::Mat<std::int16_t>>(fd, "/narrow/source",
            h5::type_conv_cb{{clamp_to_int16, nullptr}});
        check("int16 in-range value 0",          std::int16_t(0),     narrowed(0, 0));
        check("int16 in-range value 100",        std::int16_t(100),   narrowed(0, 1));
        check("int16 overflow clamped to INT16_MAX",  std::int16_t(32767),  narrowed(0, 2));
        check("int16 underflow clamped to INT16_MIN", std::int16_t(-32768), narrowed(0, 3));
    }

    // ---------------------------------------------------------------------
    // 6. DXPL composition — chunked, gzip-compressed dataset with a
    //    data-transform on write. Demonstrates that h5::data_transform
    //    composes with the rest of the dxpl/dcpl knob set via the same
    //    `|` operator the other examples use.
    // ---------------------------------------------------------------------
    {
        arma::mat M(8, 16); M.fill(2.5);
        h5::write(fd, "/composed/chunked_gzip", M,
                  h5::chunk{4, 8} | h5::gzip{6},
                  h5::data_transform{"x * 4"});                 // disk = 10.0
        auto back = h5::read<arma::mat>(fd, "/composed/chunked_gzip");
        check("transform composed with chunk + gzip on write", 10.0, back(3, 5));
    }

    std::cout << "\n"
              << (errors == 0 ? "✔ all checks passed"
                              : "✘ some checks failed")
              << ", errors=" << errors << "\n";
    return errors;
}
