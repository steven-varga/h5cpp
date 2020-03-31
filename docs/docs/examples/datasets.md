## Datasets[^1]
A dataset is stored in a file in two parts: a header and a data array.

The header contains information that is needed to interpret the array portion of the dataset, as well as metadata (or pointers to metadata) that describes or annotates the dataset. Header information includes the name of the object, its dimensionality, its number-type, information about how the data itself is stored on disk, and other information used by the library to speed up access to the dataset or maintain the file's integrity.

There are four essential classes of information in any header: name, datatype, dataspace, and storage layout:

* **Name** A dataset name is a sequence of alphanumeric ASCII characters.
* **Datatype** HDF5 allows one to define many different kinds of datatypes. There are two categories of datatypes:
	* atomic datatype: currently only **NATIVE** byte order supported on H5CPP
	* compound datatypes: see h5cpp compiler assisted reflection

* **Dataspace** A dataset dataspace describes the dimensionality of the dataset. The dimensions of a dataset can be fixed (unchanging), or they may be 	unlimited, which means that they are extendible (i.e. they can grow larger). 

* **Storage layout** The HDF5 format makes it possible to store data in a variety of ways. The default storage layout format is contiguous, meaning that data is stored in the same linear way that it is organized in memory. Two other storage layout formats are currently defined for HDF5: compact, and chunked.
	* **Chunked storage** involves dividing the dataset into equal-sized "chunks" that are stored separately. Chunking **has three important benefits**.
		1. It makes it possible to achieve good performance when accessing subsets of the datasets, even when the subset to be chosen is orthogonal to the normal storage order of the dataset.
		2. It makes it possible to compress large datasets and still achieve good performance when accessing subsets of the dataset.
		3. It makes it possible efficiently to extend the dimensions of a dataset in any direction.
	* **Compact storage is** used when the amount of data is small and can be stored directly in the object header. And is **NOT SUPPORTED by H5CPP** directly

### Dataspace
Properties of a dataspace consist of the rank (number of dimensions) of the data array, the actual sizes of the dimensions of the array, and the maximum sizes of the dimensions of the array. For a fixed-dimension dataset, the actual size is the same as the maximum size of a dimension. When a dimension is unlimited, the maximum size is set to the value H5P_UNLIMITED. 
A dataspace can also describe portions of a dataset, making it possible to do partial I/O operations on selections. Selection is supported by the dataspace interface (H5S). Given an n-dimensional dataset, there are currently four ways to do partial selection:

* Select a logically contiguous n-dimensional hyperslab.
* Select a non-contiguous hyperslab consisting of elements or blocks of elements (hyperslabs) that are equally spaced.

List to describe dimensions of a dataset:

* `h5::current_dims{i,j,k,..}` - actual dimension `i,j,k \in {1 - max}`
* `h5::max_dims{...}` - maximum dimension, use `H5S_UNLIMITED` for infinite 
* `h5::chunk{...}` - define block size, clever blocking arrangement increases throughout

List how to select from datasets for read or write:

* `h5::offset{...}` - start coordinates of data selection
* `h5::stride{...}` - every `n` considered 
* `h5::block{...}` - every `m` block is considered
* `h5::count{...}` - the amount of data 

**Note:** `h5::stride`, `h5::block` and scatter - gather operations doesn't work when `h5::high_throughput` set, due to performance reasons.






[^1]: Lifted from HDF5 CAPI documentation

