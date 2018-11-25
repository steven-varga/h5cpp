<!---
 Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->


Linear Algebra Libraries supported by  H5CPP   {#link_linalg}
==============================================================

HDF5 CPP is to simplify object persistence by implementing **CREATE,READ,WRITE,APPEND** operations on **fixed** or **variable length** N dimensional arrays.
This header only implementation supports [raw pointers][99] | [armadillo][100] | [eigen3][102] | [blaze][106] | [blitz++][103] |  [it++][104] | [dlib][105] |  [uBlas][101] | [std::vector][1]
by directly operating on the underlying data-store, avoiding intermediate/temporary memory allocations.
The api [is doxygen documented][202], furnished with an optional [h5cpp compiler](@ref link_h5cpp_compiler) for POD struct support,
detailed [examples][201], as well as [profiled][200].

Storage Layout: [Row / Column ordering][200]
=======================================
H5CPP guarantees zero copy, platform and system independent correct behaviour between supported linear algebra Matrices.
In linear algebra the de-facto standard is column major ordering which is same Fortran uses for array layout. However this is changing and many of the above systems support row-major ordering or provide only support for it. 
To add insult to injury HDF5 container has C layout, which leads to: 'What do I have to do if I am reading a Matlab/Julia/R hdf5 matrix or cube into a row major ordered Eigen Matrix?.

Answer: H5CPP auto transpose takes care of the details by transposing Column Major Matrices during serialization so the persisting object is always an Row Major order. To fix this, you'd have to transpose M matrix, with a [naive implementation][201] and a huge dataset you'd be in trouble. Luckily this operation in H5CPP has no additional computational cost, no loss of bandwidth either thanks to chunk/block storage system: only the coordinates of the chunks are swapped, resulting in ZERO COPY operation.




[99]: https://en.wikipedia.org/wiki/C_(programming_language)#Pointers
[100]: http://arma.sourceforge.net/
[101]: http://www.boost.org/doc/libs/1_66_0/libs/numeric/ublas/doc/index.html
[102]: http://eigen.tuxfamily.org/index.php?title=Main_Page#Documentation
[103]: https://sourceforge.net/projects/blitz/
[104]: https://sourceforge.net/projects/itpp/
[105]: http://dlib.net/linear_algebra.html
[106]: https://bitbucket.org/blaze-lib/blaze
[107]: https://github.com/wichtounet/etl

[200]: https://en.wikipedia.org/wiki/Row-_and_column-major_order
[201]: https://en.wikipedia.org/wiki/Row-_and_column-major_order#Transposition
