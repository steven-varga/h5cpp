#ifndef COMMON_H
#define COMMON_H

#define H5FILE_NAME   "compound.h5"
#define DATASETNAME   "ArrayOfStructures"
#define LENGTH        10
#define RANK          1

typedef struct s1_t {
  int    a;
  float  b;
  double c;
} s1_t;

typedef struct s2_t {
  double c;
  int    a;
} s2_t;

#endif
