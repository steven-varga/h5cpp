### About the Examples
You find examples for most implemented features on the project GitHub page, or, if installed from a package manager, in `/usr/local/share/h5cpp`. Only `cmake`, a C++17 compatible toolchain, and a copy of the HDF5 library (1.10.x) are required to run them. The examples can be built with `cmake` or from traditional makefiles.

### [container][11]
This example demonstrates how to create and manipulate HDF5 containers with H5CPP
 [read on HDF5 container here][104].

### [groups][10]
Akin to files in file systems, datasets in HDF5 containers are organized in a tree-like data structure, where each non-terminal node in the path is called a group. These examples illustrate how to manipulate groups, including how to add attributes.
You can [read more about groups here][102].

### [datasets][10]
This example is a guide on how to create HDF5 datasets, and control their properties. You can [read more on datasets here][103].

### [attributes][10]
Objects may have additional information attached to them via so-called attributes ("key-value pairs"). Currently only datasets `h5::ds_t` are supported, but at some point this will be extended to `h5::gr_t` and `h5::dt_t`. Attributes are intended to record metadata about objects. While (as of HDF5 1.8) there is no restriction on their size, they can be accessed only in an atomic fashion, i.e., selection-based I/O is not supported for HDF5 attributes. You can [read more on attributes here][101].

### [sparse][11]
Sparse matrices are ubiquitous in engineering. For low fill-rate matrices, various storage formats are explored.
[Read more on sparse matrices here][105].


### [basics][11]
This example demonstrates data descriptors, their properties, and how to work with them.

### [before/after][12]
This example provides an API comparison between the [C API][13] and [H5CPP][14] for persisting a compound datatype. While the example uses the `h5cpp` compiler, the [generated.hpp][15] header file is provided for convenience. Written by Gerd Heber, The HDFGroup


### [compound][16]
This is another example for persisting Plain Old Data (POD) Struct types, where a [more complex struct][17] was chosen to show that the compiler can handle arbitrarily nested types, and to give a glimpse of the quality of the [generated header][18] file.

### [linalg][19]
This includes all the linear algebra-related examples for various systems. If you are a data-scientist/engineer working with massive HDF5 datasets, probably this is where you want to start.

### [mpi][20]
The Message Passing Interface (MPI) is de facto standard for parallel application development on HPC clusters and supercomputers. This example demonstrates how to persist data to a parallel file system with [collective][21] and [independent][22] IO requests. This section is for people who intend to write code for MPI based systems.

### [multi translation unit][23] - `h5cpp` compiler
Projects are rarely small. To break such projects into different translation units is a natural way of handling complexity for compiled systems. This example shows you how to organize a [makefile][24] and how to write the [program file][25] so that it can work correctly with the `h5cpp` source code transformation tool.


### [packet table][26]
Streams of packets from sensor networks, stock exchanges, ... need  a high-performance event processor. This [example shows][27] you how simple it is to persist a variety record streams into an HDF5 datasets. Supported objects include:

* integral types -- buffered into blocks
* POD structs 

The packet table is also known to work with matrices, vectors, etc...

### [raw memory][28]
The example illustrates how to save data from memory locations, demonstrates the use of filtering algorithms such as
`h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9}`, and how to set a fill value with `h5::fill_value<short>{42}`


### [string][29]
This is a brief [example][30] of how to save a set of strings with `h5::utf8` encoding


### [transform][31]
This is a simple example of how to transform/change data within the data transfer buffer of the HDF5 C library. The purpose of this feature is 
to change the dataset transparently before loading, or saving.

### [google test][400]
Integrity tests are with `google-test` framework and placed under `test/` directory


[10]: https://github.com/steven-varga/h5cpp/tree/master/examples/attributes
[11]: https://github.com/steven-varga/h5cpp/tree/master/examples/basics
[12]: https://github.com/steven-varga/h5cpp/tree/master/examples/before-after
[13]: https://github.com/steven-varga/h5cpp/blob/master/examples/before-after/compound.c
[14]: https://github.com/steven-varga/h5cpp/blob/master/examples/before-after/compound.cpp
[15]: https://github.com/steven-varga/h5cpp/blob/master/examples/before-after/generated.h
[16]: https://github.com/steven-varga/h5cpp/tree/master/examples/compound
[17]: https://github.com/steven-varga/h5cpp/blob/master/examples/compound/struct.h
[18]: https://github.com/steven-varga/h5cpp/blob/master/examples/compound/generated.h
[19]: https://github.com/steven-varga/h5cpp/tree/master/examples/linalg
[20]: https://github.com/steven-varga/h5cpp/tree/master/examples/mpi
[21]: https://github.com/steven-varga/h5cpp/blob/master/examples/mpi/collective.cpp
[22]: https://github.com/steven-varga/h5cpp/blob/master/examples/mpi/independent.cpp
[23]: https://github.com/steven-varga/h5cpp/tree/master/examples/multi-tu
[24]: https://github.com/steven-varga/h5cpp/blob/master/examples/multi-tu/Makefile
[25]: https://github.com/steven-varga/h5cpp/blob/master/examples/multi-tu/tu_01.cpp
[26]: https://github.com/steven-varga/h5cpp/tree/master/examples/packet-table
[27]: https://github.com/steven-varga/h5cpp/blob/master/examples/packet-table/packettable.cpp
[28]: https://github.com/steven-varga/h5cpp/tree/master/examples/raw_memory
[29]: https://github.com/steven-varga/h5cpp/blob/master/examples/string
[30]: https://github.com/steven-varga/h5cpp/blob/master/examples/string/string.cpp
[31]: https://github.com/steven-varga/h5cpp/tree/master/examples/transform
[32]: https://github.com/steven-varga/h5cpp/blob/master/examples/transform/transform.cpp




[101]: examples/attributes.md 
[102]: examples/groups.md 
[103]: examples/datasets.md 
[104]: examples/container.md 
[105]: examples/sparse-matrix.md

[400]: test.md 
