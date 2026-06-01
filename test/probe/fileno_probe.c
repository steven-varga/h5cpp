// Is H5Fget_fileno the right per-physical-file key? Two opens of the SAME file
// in one process should share a fileno (HDF5 shares the underlying file struct),
// while a different file differs. This decides whether a fileno-keyed registry
// gives one collector per *physical* file (correct) vs a per-handle owner that
// would give two collectors for two opens of one file (unsafe, threadsafe-OFF).
#include <hdf5.h>
#include <stdio.h>

int main(void) {
    unsigned maj,min,rel; H5get_libversion(&maj,&min,&rel);
    printf("HDF5 %u.%u.%u\n\n", maj,min,rel);

    H5Fclose(H5Fcreate("/tmp/fn_a.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));
    H5Fclose(H5Fcreate("/tmp/fn_b.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT));

    hid_t a1 = H5Fopen("/tmp/fn_a.h5", H5F_ACC_RDWR, H5P_DEFAULT);
    hid_t a2 = H5Fopen("/tmp/fn_a.h5", H5F_ACC_RDWR, H5P_DEFAULT);  // same file, 2nd open
    hid_t b1 = H5Fopen("/tmp/fn_b.h5", H5F_ACC_RDWR, H5P_DEFAULT);  // different file

    unsigned long fa1=0, fa2=0, fb1=0;
    H5Fget_fileno(a1,&fa1); H5Fget_fileno(a2,&fa2); H5Fget_fileno(b1,&fb1);

    printf("open a #1 : hid=%ld fileno=%lu\n", (long)a1, fa1);
    printf("open a #2 : hid=%ld fileno=%lu\n", (long)a2, fa2);
    printf("open b    : hid=%ld fileno=%lu\n", (long)b1, fb1);
    printf("\n");
    printf("a#1 vs a#2 : hid %s , fileno %s\n",
           a1==a2 ? "SAME" : "different",
           fa1==fa2 ? "SAME (one physical file)" : "different");
    printf("a   vs b   : fileno %s\n", fa1==fb1 ? "same" : "different (distinct files)");

    // consistency: file id reached from a dataset matches the open's fileno
    hid_t sp = H5Screate_simple(1,(hsize_t[]){4},NULL);
    hid_t ds = H5Dcreate2(a1,"d",H5T_NATIVE_INT,sp,H5P_DEFAULT,H5P_DEFAULT,H5P_DEFAULT);
    hid_t fid = H5Iget_file_id(ds);
    unsigned long fds=0; H5Fget_fileno(fid,&fds);
    printf("\nfrom dataset: H5Iget_file_id->fileno=%lu  %s open-a fileno\n",
           fds, fds==fa1 ? "MATCHES" : "DIFFERS");

    H5Dclose(ds); H5Sclose(sp); H5Fclose(fid);
    H5Fclose(a1); H5Fclose(a2); H5Fclose(b1);
    return 0;
}
