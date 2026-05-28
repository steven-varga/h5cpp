
#include <iostream>
#include <string>
#include <vector>
#include <h5cpp/all>

// =========================================================================
// h5cpp string round-trip demo.
//
// Every C++ string type h5cpp binds to HDF5 is exercised here with an
// explicit readback check. On-disk encoding is always:
//
//   H5T_C_S1  +  H5T_VARIABLE  +  H5T_STR_NULLTERM
//
// for std::string / std::string_view / const char*. The cset on disk is
// H5T_CSET_ASCII for the dataset-creation path and H5T_CSET_UTF8 for the
// attribute path; both accept raw UTF-8 byte content unchanged. char[N]
// is fixed-length only inside compounds / attributes — at the dataset
// top level it routes through the pointer overload and refuses without
// an explicit h5::count{}. See README.md for the full type-to-HDF5 map
// and the rationale behind each decision.
// =========================================================================

namespace {
    int errors = 0;

    template <class A, class B>
    void check(const char* tag, const A& expected, const B& got) {
        const bool ok = (expected == got);
        if (!ok) ++errors;
        std::cout << (ok ? "✔ ok      " : "✘ failed  ")
                  << tag << "\n";
    }
}

int main() {
    h5::fd_t fd = h5::create("string.h5", H5F_ACC_TRUNC);

    // ---------------------------------------------------------------------
    // 1. Scalar std::string — the canonical path.
    //    access_traits_t::kind == text  →  H5T_C_S1 VLEN, scalar dataspace.
    // ---------------------------------------------------------------------
    {
        std::string greeting = "hello, world";
        h5::write(fd, "/scalar/greeting", greeting);
        auto back = h5::read<std::string>(fd, "/scalar/greeting");
        check("std::string scalar (VLEN)", greeting, back);
    }

    // ---------------------------------------------------------------------
    // 2. Non-ASCII UTF-8 content survives the byte path.
    //    HDF5 stores VLEN strings as opaque bytes; Greek + Japanese +
    //    emoji + math symbols all round-trip unchanged.
    // ---------------------------------------------------------------------
    {
        std::string utf8 = "αβγ / こんにちは / 🚀 / ∂ / ∑ / ∞";
        h5::write(fd, "/scalar/utf8", utf8);
        auto back = h5::read<std::string>(fd, "/scalar/utf8");
        check("non-ASCII UTF-8 content (Greek / Japanese / emoji / math)",
              utf8, back);
    }

    // ---------------------------------------------------------------------
    // 3. std::vector<std::string> — bulk VLEN-string dataset.
    //    storage_representation_t::vlen_text_dataset path.
    // ---------------------------------------------------------------------
    {
        std::vector<std::string> rows =
            h5::utils::get_test_data<std::string>(20);
        h5::write(fd, "/bulk/rows", rows);
        auto back = h5::read<std::vector<std::string>>(fd, "/bulk/rows");
        check("std::vector<std::string> dataset (20 rows)", rows, back);
    }

    // ---------------------------------------------------------------------
    // 4. Partial-IO with h5::offset / h5::count.
    //    The VLEN-string read path narrows the file_space selection to the
    //    requested hyperslab and sizes the char* relay to match — same
    //    pattern as the numeric pointer-read path. h5::count means
    //    "total elements requested" in h5cpp's wrapper convention, so
    //    `offset=5, count=8` returns rows[5..12] contiguously.
    // ---------------------------------------------------------------------
    {
        auto full = h5::read<std::vector<std::string>>(fd, "/bulk/rows");
        std::vector<std::string> expected(full.begin() + 5, full.begin() + 13);
        auto sub  = h5::read<std::vector<std::string>>(fd, "/bulk/rows",
            h5::offset{5}, h5::count{8});
        check("partial IO via offset=5, count=8 (contiguous slice)",
              expected, sub);
    }

    // ---------------------------------------------------------------------
    // 5. Scalar std::string as an HDF5 attribute on a dataset.
    //    Same VLEN encoding; the awrite path uses H5T_CSET_UTF8 explicitly.
    // ---------------------------------------------------------------------
    {
        h5::ds_t ds = h5::open(fd, "/bulk/rows");
        h5::awrite(ds, "author",  std::string("Steven Varga"));
        h5::awrite(ds, "license", std::string("MIT"));

        auto author  = h5::aread<std::string>(ds, "author");
        auto license = h5::aread<std::string>(ds, "license");
        check("scalar std::string attribute",
              std::string("Steven Varga"), author);
        check("ASCII attribute under UTF-8 cset",
              std::string("MIT"), license);
    }

    // ---------------------------------------------------------------------
    // 6. std::vector<std::string> as an HDF5 attribute.
    //    Rank-1 VLEN-string attribute, same storage_representation branch.
    // ---------------------------------------------------------------------
    {
        h5::ds_t ds = h5::open(fd, "/bulk/rows");
        std::vector<std::string> tags = {"alpha", "beta", "gamma", "delta"};
        h5::awrite(ds, "tags", tags);
        auto back = h5::aread<std::vector<std::string>>(ds, "tags");
        check("std::vector<std::string> attribute (4 tags)", tags, back);
    }

    // ---------------------------------------------------------------------
    // 7. Operator-= sugar from H5Awrite.hpp:289-296 — same path, terser.
    //    ds["name"] = value resolves to h5::awrite(ds, "name", value).
    // ---------------------------------------------------------------------
    {
        h5::ds_t ds = h5::open(fd, "/bulk/rows");
        ds["origin"] = std::string("vargalabs/h5cpp");
        auto back = h5::aread<std::string>(ds, "origin");
        check("ds[\"name\"] = string operator-= sugar",
              std::string("vargalabs/h5cpp"), back);
    }

    // ---------------------------------------------------------------------
    // 8. The STL pretty-printer from H5Uall.hpp.
    //    H5CPP_CONSOLE_WIDTH truncation keeps the 20-row vector readable.
    // ---------------------------------------------------------------------
    {
        std::cout << "\npretty-print of /bulk/rows via h5cpp's STL streamer:\n  ";
        auto rows = h5::read<std::vector<std::string>>(fd, "/bulk/rows");
        std::cout << rows << "\n";
    }

    std::cout << "\n"
              << (errors == 0 ? "✔ all checks passed"
                              : "✘ some checks failed")
              << ", errors=" << errors << "\n";
    return errors;
}
