#!/usr/bin/env python
# -*- coding: utf-8 -*-

import logging
import h5py
import scipy.sparse as ss
import numpy as np
import h5sparse


f = h5py.File('sparse.h5', 'w')


dt = h5py.enum_dtype({"RED": 0, "GREEN": 1, "BLUE": 42}, basetype='i')
h5py.check_enum_dtype(dt)
ds = f.create_dataset("EnumDS", (100,100), dtype=dt)
ds.dtype.kind


sparse_matrix = ss.csr_matrix(
    [[0, 1, 0],
    [0, 0, 2],
    [0, 0, 0],
    [3, 4, 0]], dtype=np.float64)

print( sparse_matrix )
with h5sparse.File("test.h5","w") as h5f:
    h5f.create_dataset('my-matrix', data=sparse_matrix)

