# Sparse Matrix[^1]
<div id="object" style="float: right">
	![Object](../pix/sparse-csr.svg)
</div>

The fill rate of a matrix is a ration between non-zero and zero elements. If the latter significantly outweighs the former then we speak of Sparse Matrices. Depending on the sparsity pattern some storage format are more efficient than others. Nevertheless a sparse matrix is an object of multiple fields as opposed to a single contagious memory location with homogeneous type.

Netlib considers the following [sparse storage formats][109]:

|description                             | `h5::dapl_t`        |
|--------------------|:----------------------------------------|
|[Compressed Sparse Row][110]            | `h5::sparse::csr`   |
|[Compressed Sparse Column][111]         | `h5::sparse::csc`   |
|[Block Compressed Sparse Storage][112]  | `h5::sparse::bcrs`  |
|[Compressed Diagonal Storage][113]      | `h5::sparse::cds`   |
|[Jagged Diagonal Storage][114]          | `h5::sparse::jds`   |
|[Skyline Storage][115]                  | `h5::sparse::ss`    |




## Multi Dataset Storage Format



## Single Dataset Storage Format
TODO: write code and documentation


# Interop With Other Systems
## Python
[Alex Wolf discusses HDF5][1] and Sparse Matrix formats, and [h5py][11] nor [pytables][10] support sparse matrices.
### PyTables
has no direct support to save / load sparse matrices

```python
import scipy.sparse as sp_sparse
import tables

with tables.open_file(filename, 'r') as f:
	mat_group = f.get_node(f.root, 'matrix')
	data = getattr(mat_group, 'data').read()
	indices = getattr(mat_group, 'indices').read()
	indptr = getattr(mat_group, 'indptr').read()
	shape = getattr(mat_group, 'shape').read()
	matrix = sp_sparse.csc_matrix((data, indices, indptr), shape=shape)
```

### H5PY
```python
```

### h5sparse
```
```


## Julia
<div id="object" style="float: left;">
	![Object](../pix/julia-sparse-mat.png)
</div>
[SparseArrays][14] uses Compressed Sparse Column format and the official JLD format can save and load sparse matrices. Less fortunate how the data sets are organized within the HDF5 container, instead the actual data is placed under `_refs` directory. The screen shot shows A,B sparse matrices saved in Julia, and a Pyhton `h5sparse` to compare.  On the bright side the julia HDF5 package is feature full, it is possible loading sparse matrices to H5PY.

```
using JLD, SparseArrays

A = sprand(Float64, 10,20, 0.1)
B = sprand(Float64, 10,20, 0.1)
@save "interop.h5" "data-01/A" A "data-02/B" B 
```


## R



# Bio Informatics
## [Loompy][13]
is an efficient file format for large omics datasets. Loom files contain a main matrix, optional additional layers, a variable number of row and column annotations, and sparse graph objects. Under the hood, Loom files are HDF5 and can be opened from many programming languages, including Python, R, C, C++, Java, MATLAB, Mathematica, and Julia.

## [10x Genomics][12]

The top level of the file contains a single HDF5 group, called matrix, and metadata stored as HDF5 attributes. Within the matrix group are datasets containing the dimensions of the matrix, the matrix entries, as well as the features and cell-barcodes associated with the matrix rows and columns, respectively.
format
--------
|Column  |	  Type|	Description                                                                                                          |
|--------|--------|----------------------------------------------------------------------------------------------------------------------|
|barcodes| string | Barcode sequences and their corresponding GEM wells (e.g. AAACGGGCAGCTCGAC-1)                                        |
|data	 | uint32 |	Nonzero UMI counts in column-major order                                                                             |
|indices | uint32 | Zero-based row index of corresponding element in data                                                                |
|indptr  | uint32 |	Zero-based index into data / indices of the start of each column, i.e., the data corresponding to each barcode sequence |
|shape	 | uint64 |	Tuple of (# rows, # columns) indicating the matrix dimensions                                                        |




[1]: http://www.falexwolf.de/blog/171212_sparse_matrices_with_h5py/
[10]: http://www.pytables.org/
[11]: http://www.h5py.org/
[12]: https://support.10xgenomics.com/single-cell-gene-expression/software/pipelines/latest/advanced/h5_matrices
[13]: http://loompy.org/
[14]: https://docs.julialang.org/en/v1/stdlib/SparseArrays/#man-csc-1

[99]: https://en.wikipedia.org/wiki/C_(programming_language)#Pointers
[100]: http://arma.sourceforge.net/
[101]: http://www.boost.org/doc/libs/1_66_0/libs/numeric/ublas/doc/index.html
[102]: http://eigen.tuxfamily.org/index.php?title=Main_Page#Documentation
[103]: https://sourceforge.net/projects/blitz/
[104]: https://sourceforge.net/projects/itpp/
[105]: http://dlib.net/linear_algebra.html
[106]: https://bitbucket.org/blaze-lib/blaze
[107]: https://github.com/wichtounet/etl
[108]: http://www.netlib.org/utk/people/JackDongarra/la-sw.html
[109]: http://www.netlib.org/utk/people/JackDongarra/etemplates/node372.html
[110]: http://www.netlib.org/utk/people/JackDongarra/etemplates/node373.html
[111]: http://www.netlib.org/utk/people/JackDongarra/etemplates/node374.html
[112]: http://www.netlib.org/utk/people/JackDongarra/etemplates/node375.html
[113]: http://www.netlib.org/utk/people/JackDongarra/etemplates/node376.html
[114]: http://www.netlib.org/utk/people/JackDongarra/etemplates/node377.html
[115]: http://www.netlib.org/utk/people/JackDongarra/etemplates/node378.html
[116]: https://docs.scipy.org/doc/scipy/reference/sparse.html


[^1]: Material based on Netlib Documentation

