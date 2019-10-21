## Sparse Matrix[^1]
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


### Multi Dataset Storage Format



### Single Dataset Storage Format
TODO: write code and documentation


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



[^1]: Material based on Netlib Documentation

