
#include "hdf5.h"

void main() {
	hsize_t dims[] = {7, 11};
	hsize_t cdims[] = {3, 3};
	hsize_t offset[] = {0, 0};
	hid_t file, dcpl, fspace, dset;
	int foo[] = {1,2,3,4,5,6,7,8,9};

	file = H5Fcreate("chunk.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	dcpl = H5Pcreate(H5P_DATASET_CREATE);
	H5Pset_chunk(dcpl, 2, cdims);
	fspace = H5Screate_simple(2, dims, NULL);
	dset = H5Dcreate(file, "foo", H5T_NATIVE_INT, fspace, H5P_DEFAULT, dcpl, H5P_DEFAULT);
	H5Dwrite_chunk(dset, H5P_DEFAULT, 0, offset, 9*sizeof(int), &foo);

	offset[0] = 3; offset[1] = 9;
	H5Dwrite_chunk(dset, H5P_DEFAULT, 0, offset, 6*sizeof(int), &foo);

	H5Sclose(fspace);
	H5Pclose(dcpl);
	H5Fclose(file);
}

/* 

 (0,0): 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0,
 (1,0): 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0,
 (2,0): 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0,
 (3,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, x
 (4,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, x
 (5,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 8, x
 (6,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0

And I get this result when specifying 6*sizeof(int):

 (0,0): 1, 2, 3, 0, 0, 0, 0, 0, 0, 0, 0,
 (1,0): 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0,
 (2,0): 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 0,
 (3,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, x
 (4,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 5, x
 (5,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 (6,0): 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
*/
