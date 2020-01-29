# H5CPP documentation (Sandbox)

[H5CPP](http://h5cpp.org) is template only persistence for modern C++ with compiler assisted reflection. To take the notch a step higher, this DEVELOPER BRANCH comes with  improved documentation in addition to:

* General C++ Class persistence PLANNED
* Sparse Matrix Support IN-PROGRESS
* Full STL Support IN-PROGRESS
* More Examples: IN-PROGRESS


The pages maybe viewed live at [SANDBOX](http://sandbox.h5cpp.org)

**FOCUS:** Currently I am working on a thorough treatment of Sparse Matrices, if you are a researcher working with massive sparse datasets I am interested in your input.

Netlib considers the following [sparse storage formats][109]:

|description                             | `h5::dapl_t`        |
|--------------------|:----------------------------------------|
|[Compressed Sparse Row][110]            | `h5::sparse::csr`   |
|[Compressed Sparse Column][111]         | `h5::sparse::csc`   |
|[Block Compressed Sparse Storage][112]  | `h5::sparse::bcrs`  |
|[Compressed Diagonal Storage][113]      | `h5::sparse::cds`   |
|[Jagged Diagonal Storage][114]          | `h5::sparse::jds`   |
|[Skyline Storage][115]                  | `h5::sparse::ss`    |
=======
Tested against:
-----------------
- gcc 7.4.0, gcc 8.3.0, gcc 9.0.1
- clang 6.0

**Note:** the preferred compiler is gcc, however there is work put in to broaden the support for major modern C++ compilers. Please contact me if there is any shortcomings.



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



