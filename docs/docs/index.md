<!---
 Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->

### Easy-to-use  [HDF5][hdf5] C++ templates for Serial and Parallel HDF5  
<p align='justify'>
The [Hierarchical Data Format][hdf5], prevalent in high performance scientific computing, sits directly on top of sequential or parallel file systems, providing block and stream operations on standardized or custom binary/text objects. Popular scientific computing platforms such as Python, R, Matlab, Fortran,  Julia [and many more...] come with the necessary libraries to read and write HDF5 datasets. The present effort simplifies the interaction with [popular linear algebra libraries][304], provides [compiler assisted seamless object persistence][303], Standard Template Library support, and comes equipped with a novel [error handling architecture][400].
</p> 
<p align='justify'>
H5CPP is a novel approach to  persistence in the field of machine learning, it provides high performance sequential and block access to HDF5 containers through modern C++.
</p>
All H5CPP **file and dataset IO** descriptors implement the [raii idiom][301] and close underlying resource when going out of scope. They can be passed seamlessly to HDF5 CAPI calls when implicit conversion is enabled. Similarly, templates can take CAPI `hid_t` identifiers as arguments where applicable, provided the chosen conversion policy allows it. See [conversion policy][306] for details.

The system supports CRUD like IO operators: [`h5::create`][700], [`h5::read`][701], [`h5::write`][702],[`h5::append`][703]


[hdf5]: https://support.hdfgroup.org/HDF5/doc/H5.intro.html
[1]: http://en.cppreference.com/w/cpp/container/vector
[2]: http://arma.sourceforge.net
[4]: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5Front.html
[5]: https://support.hdfgroup.org/HDF5/release/obtain5.html
[6]: http://eigen.tuxfamily.org/index.php?title=Main_Page
[7]: http://www.boost.org/doc/libs/1_65_1/libs/numeric/ublas/doc/matrix.htm
[8]: https://julialang.org/
[9]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_row_.28CSR.2C_CRS_or_Yale_format.29
[10]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_column_.28CSC_or_CCS.29
[11]: https://en.wikipedia.org/wiki/List_of_numerical_libraries#C++
[12]: http://en.cppreference.com/w/cpp/concept/StandardLayoutType
[40]: https://support.hdfgroup.org/HDF5/Tutor/HDF5Intro.pdf
[99]: https://en.wikipedia.org/wiki/C_(programming_language)#Pointers
[100]: http://arma.sourceforge.net/
[101]: http://www.boost.org/doc/libs/1_66_0/libs/numeric/ublas/doc/index.html
[102]: http://eigen.tuxfamily.org/index.php?title=Main_Page#Documentation
[103]: https://sourceforge.net/projects/blitz/
[104]: https://sourceforge.net/projects/itpp/
[105]: http://dlib.net/linear_algebra.html
[106]: https://bitbucket.org/blaze-lib/blaze
[107]: https://github.com/wichtounet/etl
[200]: architecture.md#profileing
[201]: http://h5cpp.org/examples.html
[202]: http://h5cpp.org/modules.html
[400]: https://www.meetup.com/Chicago-C-CPP-Users-Group/events/250655716/
[401]: https://www.hdfgroup.org/2018/07/cpp-has-come-a-long-way-and-theres-plenty-in-it-for-users-of-hdf5/
[301]: architecture.md#raii
[302]: architecture.md#error-handling
[303]: compiler.md#the-idea
[304]: architecture.md#linear-algebra
[305]: install.md
[306]: architecture.md#conversion-policy
[307]: architecture.md#property-lists
[308]: architecture.md#custom-filter-pipeline
[400]: architecture.md#error-handling
[500]: http://h5cpp.org/md__home_steven_Documents_projects_h5cpp_docs_pages_blog.html
[501]: http://h5cpp.org/modules.html
[502]: http://h5cpp.org/examples.html
[503]: http://h5cpp.org/independent_8cpp-example.html
[504]: http://h5cpp.org/collective_8cpp-example.html
[505]: http://h5cpp.org/throughput_8cpp-example.html

[601]: architecture.md#file-creation-property-list
[602]: architecture.md#file-access-property-list
[603]: architecture.md#link-creation-property-list
[604]: architecture.md#dataset-creation-property-list
[605]: architecture.md#dataset-access-property-list
[606]: architecture.md#dataset-transfer-property-list
[700]: architecture.md#create
[701]: architecture.md#read
[702]: architecture.md#write
[703]: architecture.md#append
[704]: architecture.md#
[705]: architecture.md#
