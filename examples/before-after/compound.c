#include "compound.h"
#include "hdf5.h"




int
main(void)
{
  s1_t s1[LENGTH];
  s2_t s2[LENGTH];
  int  i;

  for (i = 0; i< LENGTH; i++) {
      s1[i].a = i;
      s1[i].b = i*i;
      s1[i].c = 1./(i+1);
  }

  hid_t   s1_tid;
  hid_t   s2_tid;
  hid_t   fd, dataset, space;
  hsize_t dim[] = {LENGTH};
 
  space = H5Screate_simple(RANK, dim, NULL);
  s1_tid = H5Tcreate (H5T_COMPOUND, sizeof(s1_t));
  H5Tinsert(s1_tid, "a_name", HOFFSET(s1_t, a), H5T_NATIVE_INT);
  H5Tinsert(s1_tid, "c_name", HOFFSET(s1_t, c), H5T_NATIVE_DOUBLE);
  H5Tinsert(s1_tid, "b_name", HOFFSET(s1_t, b), H5T_NATIVE_FLOAT);

  fd = H5Fcreate(H5FILE_NAME, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

  dataset = H5Dcreate2(fd, DATASETNAME, s1_tid, space, H5P_DEFAULT,
                       H5P_DEFAULT, H5P_DEFAULT);

  H5Dwrite(dataset, s1_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, s1);

  H5Tclose(s1_tid);
  H5Sclose(space);
  H5Dclose(dataset);

  s2_tid = H5Tcreate(H5T_COMPOUND, sizeof(s2_t));
  H5Tinsert(s2_tid, "c_name", HOFFSET(s2_t, c), H5T_NATIVE_DOUBLE);
  H5Tinsert(s2_tid, "a_name", HOFFSET(s2_t, a), H5T_NATIVE_INT);

  dataset = H5Dopen2(fd, DATASETNAME, H5P_DEFAULT);

  H5Dread(dataset, s2_tid, H5S_ALL, H5S_ALL, H5P_DEFAULT, s2);

  printf("reading back data previously written:\n\t");
  for (i = 0; i < LENGTH; ++i)
      printf("%g ", s2[i].c);
  printf("\n");

  H5Tclose(s2_tid);
  H5Dclose(dataset);
  H5Fclose(fd);

  return 0;
}
