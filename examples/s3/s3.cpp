/* Copyright (c) 2018-2026 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargalabs.com> */

/**
 * @file s3.cpp
 * @brief Open HDF5 files on S3 using the Read-Only S3 VFD (ROS3).
 *
 * Requires HDF5 built with ROS3 support (H5_HAVE_ROS3_VFD).
 *
 * URL form:
 *   - HDF5 >= 1.14.x   accepts the s3://bucket/key shorthand
 *   - HDF5  < 1.14     needs the explicit virtual-hosted HTTPS URL
 *                      https://<bucket>.s3.<region>.amazonaws.com/<key>
 *   The example below uses the HTTPS form so it works on both.
 *
 * Build: cmake -DH5CPP_BUILD_EXAMPLES=ON .
 * Run:   AWS_REGION=us-east-1 ./s3
 */

#include <h5cpp/all>
#include <iostream>
#include <stdexcept>

#ifndef H5_HAVE_ROS3_VFD
int main() {
    std::cerr << "This example requires HDF5 built with ROS3 VFD support.\n"
              << "Rebuild HDF5 with --enable-ros3-vfd (and libcurl + OpenSSL).\n";
    return 1;
}
#else

int main() {
    try {
        // ── 1. Unauthenticated: public bucket ───────────────────────────────
        // Reads the HDFGroup canonical h5ex_t_array.h5 example hosted by the
        // Bioconductor rhdf5 team — `/DS1` is 4 elements of [3][5] int64 array
        // (60 int64s = 480 bytes total).  This proves data bytes — not just
        // metadata — traverse the S3 → ROS3 → h5cpp pipeline.
        {
            auto fd = h5::open(
                "https://rhdf5-public.s3.eu-central-1.amazonaws.com/h5ex_t_array.h5",
                H5F_ACC_RDONLY, h5::ros3{false, "eu-central-1", "", ""});

            std::cout << "Public bucket (no auth):\n";
            for (auto& name : h5::ls(fd, "/")) std::cout << "  /" << name << "\n";

            // The on-disk datatype is H5T_ARRAY{[3][5] H5T_STD_I64LE}: 4
            // elements × 15 int64 = 480 bytes.  Read with the file's own
            // datatype to skip type conversion and prove bytes traversed S3.
            auto ds = h5::open(fd, "/DS1");
            hid_t dt = H5Dget_type(static_cast<hid_t>(ds));
            std::size_t elem_size = H5Tget_size(dt);
            auto npoints = H5Sget_simple_extent_npoints(H5Dget_space(static_cast<hid_t>(ds)));
            std::vector<int64_t> data(elem_size * npoints / sizeof(int64_t));
            H5Dread(static_cast<hid_t>(ds), dt, H5S_ALL, H5S_ALL, H5P_DEFAULT, data.data());
            H5Tclose(dt);
            std::cout << "  /DS1: read " << data.size() << " int64s ("
                      << data.size() * sizeof(int64_t) << " bytes) from S3\n";
            // Print element 0 as the canonical 3×5 grid so the reader can
            // eyeball that real data — not zero-init garbage — traversed.
            std::cout << "  element[0]:\n";
            for (int r = 0; r < 3; ++r) {
                std::cout << "    ";
                for (int c = 0; c < 5; ++c) std::cout << data[r*5 + c] << '\t';
                std::cout << '\n';
            }
        }

        // ── 2. Authenticated: long-term credentials ─────────────────────────
        // Read credentials from environment — never hard-code.
        const char* region = std::getenv("AWS_REGION");
        const char* key_id = std::getenv("AWS_ACCESS_KEY_ID");
        const char* secret = std::getenv("AWS_SECRET_ACCESS_KEY");

        if (region && key_id && secret) {
            auto fd      = h5::open(
                "https://my-private-bucket.s3.us-east-1.amazonaws.com/archive/run42.h5",
                H5F_ACC_RDONLY, h5::ros3{true, region, key_id, secret});
            auto entries = h5::ls(fd, "/");
            std::cout << "Private bucket (long-term creds):\n";
            for (auto& name : entries) std::cout << "  /" << name << "\n";

            // Normal h5cpp I/O — no S3-specific code needed after open
            auto temps = h5::read<std::vector<double>>(fd, "/sensor/temperature");
            std::cout << "  read " << temps.size() << " temperature values\n";
        } else {
            std::cout << "Set AWS_REGION, AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY "
                         "to test authenticated access.\n";
        }

#if H5FD_CURR_ROS3_FAPL_T_VERSION >= 2
        // ── 3. Temporary credentials: STS / IAM role (HDF5 >= 1.14.x) ──────
        const char* token = std::getenv("AWS_SESSION_TOKEN");
        if (region && key_id && secret && token) {
            auto fd      = h5::open(
                "https://my-secure-bucket.s3.us-east-1.amazonaws.com/data.h5",
                H5F_ACC_RDONLY, h5::ros3{true, region, key_id, secret, token});
            auto entries = h5::ls(fd, "/");
            std::cout << "Secure bucket (STS session token):\n";
            for (auto& name : entries) std::cout << "  /" << name << "\n";
        }
#endif

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
#endif
