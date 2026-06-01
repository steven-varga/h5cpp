// Probe: can a custom user property embedded in a FAPL be recovered from the
// file id via H5Fget_access_plist on HDF5 1.12.x?  This decides whether the
// per-file worker pool / collector can be carried *through the file* (FAPL) or
// must be carried on an h5cpp-side wrapper (as h5::async already does).
//
// Two mechanisms tested:
//   A) H5Pinsert2 — a temporary per-list property holding a pointer.
//   B) A registered property *class* property (H5Pregister2 on a derived class)
//      is out of scope here; we test the realistic h5cpp path (temporary insert
//      on a standard H5P_FILE_ACCESS list), which is what resolve_worker_pool
//      relies on.
#include <hdf5.h>
#include <stdio.h>
#include <stdint.h>

static const char* PROP = "h5cpp_probe_pool_ptr";

static void show(const char* what, hid_t fapl, void* expect) {
    htri_t ex = H5Pexist(fapl, PROP);
    printf("  %-34s H5Pexist=%d  => %s\n", what, (int)ex,
           ex > 0 ? "PRESENT" : "ABSENT");
    if (ex > 0) {
        void* got = NULL;
        if (H5Pget(fapl, PROP, &got) >= 0)
            printf("  %-34s H5Pget=%p expect=%p  %s\n", "", got, expect,
                   got == expect ? "MATCH" : "MISMATCH");
    }
}

int main(void) {
    unsigned maj, min, rel;
    H5get_libversion(&maj, &min, &rel);
    printf("HDF5 %u.%u.%u\n\n", maj, min, rel);

    void* sentinel = (void*)(uintptr_t)0xDEADBEEFCAFEF00DULL;

    // 1) Build a FAPL and embed the pointer via a temporary property.
    hid_t fapl = H5Pcreate(H5P_FILE_ACCESS);
    void* v = sentinel;
    herr_t e = H5Pinsert2(fapl, PROP, sizeof(void*), &v,
                          NULL, NULL, NULL, NULL, NULL, NULL);
    printf("[create-time FAPL]\n");
    printf("  H5Pinsert2 rc=%s\n", e >= 0 ? "ok" : "FAIL");
    show("on original FAPL", fapl, sentinel);

    // 2) Create a file with that FAPL.
    hid_t fid = H5Fcreate("/tmp/h5cpp_fapl_probe.h5", H5F_ACC_TRUNC,
                          H5P_DEFAULT, fapl);
    printf("\n[H5Fcreate] rc=%s\n", fid >= 0 ? "ok" : "FAIL");

    // 3) Pull the FAPL back off the file id and check survival.
    hid_t fapl2 = H5Fget_access_plist(fid);
    printf("\n[FAPL retrieved via H5Fget_access_plist] rc=%s\n",
           fapl2 >= 0 ? "ok" : "FAIL");
    show("on retrieved FAPL", fapl2, sentinel);

    // 4) Also check whether the two FAPLs are even the \"same\" class/equal.
    htri_t eq = H5Pequal(fapl, fapl2);
    printf("\n  H5Pequal(original, retrieved) = %d (%s)\n", (int)eq,
           eq > 0 ? "equal" : "NOT equal");

    H5Pclose(fapl);
    H5Pclose(fapl2);
    H5Fclose(fid);
    return 0;
}
