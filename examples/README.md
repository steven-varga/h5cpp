## Examples 

the content of this directory gets installed `${INSTALLDIR}/share/h5cpp`, copy the directory into a temporary place then execute cmake **in-directory**:
`cmake ./`

## What do they do:
- attributes: new add on to guide you with attribute
- basics: help you to catch on HDF5 concepts such as descriptors and property lists
- before-after: comparative example between C API and H5CPP approach
- linalg: you find all you need to know as a scientist when working  HDF5 for modern C++
- mpi: collective and independent IO examples are in progress
- multi-tu: demonstrates how to use `h5cpp` compiler with multiple TU translation units
- optimized: work in progress
- packet-table: sensor networks, financial engineering, particle colliders require high performance stream recording. This implementation is near the actual underlying file-system throughput 300X faster than the one you find in HDF5 high level API. Not only works with non-homogeneous such as structs but also takes vectors, matrices, cubes...
- raw-memory: for the advanturous straight from memory to disk operations. Mind you all other function calls do have the same computational properties in terms of speed, or memory thanks to template meta programming (compile time evaluation). These calls are provided for cases I didn't cover, but you need it now.
- string: examples for variable length string storage. HDF5 keeps string in global heap datastructure, applying compression will not work
- transform: gives examples to change data in buffer; don't get used to it... I think you get more flexible result doing your own thing after reading the data into your application. 
- utf8: for posix users only, weird problems and constant nightmares for windows users; but hey it works fine on supercomputers, clusters
