// Does a custom property stashed on a DATASET property list survive retrieval?
// Tests four round-trips that matter for "stash the per-file collector/pool on
// the dataset" designs:
//   1. DAPL via H5Dget_access_plist on the SAME open dataset handle
//   2. DAPL after close + H5Dopen2 (fresh handle)
//   3. DCPL via H5Dget_create_plist on the SAME open dataset handle
//   4. DCPL after close + H5Dopen2 (fresh handle)
// Contrast with the FAPL, which H5Fget_access_plist reconstructs (strips custom).
#include <hdf5.h>
#include <stdio.h>
#include <stdint.h>

static const char* PROP = "h5cpp_probe_ptr";

static void check(const char* what, hid_t plist, void* expect) {
    htri_t ex = H5Pexist(plist, PROP);
    printf("  %-44s H5Pexist=%d => %s", what, (int)ex,
           ex > 0 ? "PRESENT" : "ABSENT");
    if (ex > 0) {
        void* got = NULL;
        H5Pget(plist, PROP, &got);
        printf("  (val %s)", got == expect ? "MATCH" : "MISMATCH");
    }
    printf("\n");
}

static hid_t make_with_prop(hid_t cls, void* v) {
    hid_t p = H5Pcreate(cls);
    H5Pinsert2(p, PROP, sizeof(void*), &v, NULL,NULL,NULL,NULL,NULL,NULL);
    return p;
}

int main(void) {
    unsigned maj, min, rel; H5get_libversion(&maj,&min,&rel);
    printf("HDF5 %u.%u.%u\n\n", maj, min, rel);
    void* sentinel = (void*)(uintptr_t)0xABCD1234DEADBEEFULL;

    hid_t fid = H5Fcreate("/tmp/h5cpp_dapl_probe.h5", H5F_ACC_TRUNC,
                          H5P_DEFAULT, H5P_DEFAULT);
    hid_t space = H5Screate_simple(1, (hsize_t[]){16}, NULL);

    // DAPL carries the sentinel at create time.
    hid_t dapl = make_with_prop(H5P_DATASET_ACCESS, sentinel);
    // DCPL carries the sentinel at create time (also set chunking so it's a real dcpl).
    hid_t dcpl = make_with_prop(H5P_DATASET_CREATE, sentinel);
    H5Pset_chunk(dcpl, 1, (hsize_t[]){8});

    hid_t dset = H5Dcreate2(fid, "data", H5T_NATIVE_DOUBLE, space,
                            H5P_DEFAULT, dcpl, dapl);
    printf("[H5Dcreate2] rc=%s\n", dset >= 0 ? "ok" : "FAIL");

    // 1. DAPL, same open handle
    hid_t got_dapl = H5Dget_access_plist(dset);
    check("1. DAPL via H5Dget_access_plist (same handle)", got_dapl, sentinel);

    // 3. DCPL, same open handle
    hid_t got_dcpl = H5Dget_create_plist(dset);
    check("3. DCPL via H5Dget_create_plist (same handle)", got_dcpl, sentinel);

    H5Pclose(got_dapl); H5Pclose(got_dcpl);
    H5Dclose(dset);

    // Reopen fresh — with a DEFAULT dapl (the realistic reopen case)
    hid_t dset2 = H5Dopen2(fid, "data", H5P_DEFAULT);
    hid_t re_dapl = H5Dget_access_plist(dset2);
    check("2. DAPL after close+H5Dopen2(default dapl)", re_dapl, sentinel);
    hid_t re_dcpl = H5Dget_create_plist(dset2);
    check("4. DCPL after close+H5Dopen2", re_dcpl, sentinel);

    // Bonus: reopen passing the ORIGINAL dapl (does h5cpp-style re-supply work?)
    H5Dclose(dset2);
    hid_t dset3 = H5Dopen2(fid, "data", dapl);
    hid_t re2_dapl = H5Dget_access_plist(dset3);
    check("5. DAPL after H5Dopen2(original dapl supplied)", re2_dapl, sentinel);

    H5Pclose(re_dapl); H5Pclose(re_dcpl); H5Pclose(re2_dapl);
    H5Pclose(dapl); H5Pclose(dcpl); H5Sclose(space);
    H5Dclose(dset3); H5Fclose(fid);
    return 0;
}
