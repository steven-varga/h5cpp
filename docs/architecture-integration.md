---
hide:
  - toc
---


###  :material-matrix:{.icon} Linear Algebra
H5CPP simplifies persistence for numerical objects by providing the full set of [create][create], [read][read], [write][write], and [append][append] operations for fixed-size and variable-length N-dimensional arrays. The library is header-only and supports [raw pointers][99], [Armadillo][100], [uBLAS][101], [Eigen3][102], [Blitz++][103], [IT++][104], [dlib][105], and [Blaze][106]. It operates directly on the underlying memory of supported objects, avoids unnecessary temporary allocations, and relies on copy elision where applicable when returning values.
```cpp
h5::ds_t ds = h5::open( ... ) 		// open dataset
arma::mat M(n_rows,n_cols);   		// create placeholder, data-space is reserved on the heap
h5::count_t  count{n_rows,n_cols}; 	// describe the memory region you are reading into
h5::offset_t offset{0,0}; 			// position we reasing data from
// high performance loop with minimal memory operations
for( auto i: column_indices )
    h5::read(ds, M, count, offset); // count, offset and other properties may be speciefied in any order
```

List of objects supported in EBNF:
```yacc
T := ([unsigned] ( int8_t | int16_t | int32_t | int64_t )) | ( float | double  )
S := T | c/c++ struct | std::string
ref := std::vector<S> 
    | arma::Row<T> | arma::Col<T> | arma::Mat<T> | arma::Cube<T> 
    | Eigen::Matrix<T,Dynamic,Dynamic> | Eigen::Matrix<T,Dynamic,1> | Eigen::Matrix<T,1,Dynamic>
    | Eigen::Array<T,Dynamic,Dynamic>  | Eigen::Array<T,Dynamic,1>  | Eigen::Array<T,1,Dynamic>
    | blaze::DynamicVector<T,rowVector> |  blaze::DynamicVector<T,colVector>
    | blaze::DynamicVector<T,blaze::rowVector> |  blaze::DynamicVector<T,blaze::colVector>
    | blaze::DynamicMatrix<T,blaze::rowMajor>  |  blaze::DynamicMatrix<T,blaze::colMajor>
    | itpp::Mat<T> | itpp::Vec<T>
    | blitz::Array<T,1> | blitz::Array<T,2> | blitz::Array<T,3>
    | dlib::Matrix<T>   | dlib::Vector<T,1> 
    | ublas::matrix<T>  | ublas::vector<T>
ptr 	:= T* 
accept 	:= ref | ptr 
```

The following table summarizes how the supported linear algebra backends expose contiguous storage, report element counts, and map logical dimensions to physical layout. H5CPP uses these conventions to infer shape and perform direct I/O without intermediate copies.

| backend       | data accessor     | element count | vector layout           | matrix layout (row-major) | matrix layout (column-major) | cube / tensor layout             |
| ------------- | ----------------- | ------------- | ----------------------- | ------------------------- | ---------------------------- | -------------------------------- |
| **Eigen**     | `.data()`         | `size()`      | `rows():1, cols():0`    | `rows():0, cols():1`      | `cols():0, rows():1`         | n/a                              |
| **Armadillo** | `.memptr()`       | `n_elem`      | n/a                     | n/a                       | `n_rows:0, n_cols:1`         | `n_slices:2, n_rows:0, n_cols:1` |
| **Blaze**     | `.data()`         | n/a           | `columns():1, rows():0` | `columns():1, rows():0`   | `rows():0, columns():1`      | n/a                              |
| **Blitz++**   | `.data()`         | `size()`      | `cols:1, rows:0`        | `cols:1, rows:0`          | n/a                          | `slices:2, cols:1, rows:0`       |
| **IT++**      | `._data()`        | `length()`    | `cols():1, rows():0`    | `cols():1, rows():0`      | n/a                          | n/a                              |
| **uBLAS**     | `.data().begin()` | n/a           | n/a                     | `size2():1, size1():0`    | n/a                          | n/a                              |
| **dlib**      | `&ref(0,0)`       | `size()`      | n/a                     | `nc():1, nr():0`          | n/a                          | n/a                              |

#### :material-view-column-outline:{.icon} Storage Layout: [Row- and Column-Major Ordering][200]

H5CPP preserves direct, zero-copy I/O for supported dense matrix types, while maintaining correct platform-independent storage semantics across the supported linear algebra backends. In practice, this means H5CPP respects the native memory layout of the source object instead of silently rearranging data behind the user’s back. Traditionally, dense linear algebra libraries have favored column-major layout, following the Fortran and BLAS/LAPACK convention. More recent systems often support row-major layout as well, either as a default or as a selectable storage policy. At present, H5CPP does not automatically transpose between row-major and column-major matrix layouts during I/O. For example, a column-major object such as `arma::mat` is stored according to its native memory order, and no implicit conversion to row-major layout is performed. In principle, such conversion could be implemented either through an explicit transpose step in the transfer path or through metadata that marks the object as logically transposed, similar in spirit to BLAS transpose flags. However, neither approach is directly supported by the HDF5 C API in a standard, interoperable way. For now, explicit user-controlled transpose remains the portable solution, and is supported by most linear algebra libraries.

#### :material-chart-scatter-plot:{.icon} Sparse Matrices and Vectors
For sparse objects, H5CPP follows established storage conventions from numerical computing wherever practical. The supported layouts are aligned with the familiar Netlib taxonomy:

| Description                         | `h5::dapl_t` selector |
| ----------------------------------- | --------------------- |
| [Compressed Sparse Row][110]        | `h5::sparse::csr`     |
| [Compressed Sparse Column][111]     | `h5::sparse::csc`     |
| [Block Compressed Row Storage][112] | `h5::sparse::bcrs`    |
| [Compressed Diagonal Storage][113]  | `h5::sparse::cds`     |
| [Jagged Diagonal Storage][114]      | `h5::sparse::jds`     |
| [Skyline Storage][115]              | `h5::sparse::ss`      |

At present, H5CPP provides support for Compressed Sparse Row ([CSR][csr]) and Compressed Sparse Column ([CSC][csc]). A sparse object may be represented physically either as a collection of datasets grouped under an `h5::gr_t` node—for example, with separate datasets for indices, offsets, and values—or as a single compound datatype used as a structured placeholder for the sparse representation. More specialized formats, such as block-diagonal, tridiagonal, or triangular storage, are not currently implemented. Whenever possible, H5CPP follows BLAS/LAPACK-style storage conventions so that sparse datasets remain familiar to users coming from established numerical libraries.



###  :material-language-cpp:{.icon} The STL and classes implementing the interface
While it is possible to detect specific STL container instead a broader approach is considered: any class that provides STL like interfaces
are identified with the detection idoim outlined in paper [WG21 N4421][14]. 

```cpp
template<class T, class... Args> using is_container = 
    std::disjunction<
        is_detected<has_container_t, T>,     // is an adaptor?
    std::conjunction<                        // or have these properties:
        is_detected<has_difference_t,T>, 
        is_detected<has_begin_t,T>, is_detected<has_const_begin_t,T>,
        is_detected<has_end_t,T>, is_detected<has_const_end_t,T>>
    >;
```

From a storage perspective, there are three notable categories:

* `std::vector<T>`, `std::array<T,N>` have `.data()` accessors and H5CPP can directly load/save data from the objects. For efficient partial data transfer the data transfer size must match the element size of the objects.

* `std::list<T>`, `std::forward_list<T>`, `std::deque<T>`, `std::set<T>`, `std::multiset<T>`,`std::unordered_set<T>`,`std::map<K,V>`,`std::multimap<K,V>`, `std::unordered_multimap<K,V>`
don't have direct access to the underlying memory store and the provided iterators are used for data transfer between memory and disk. The IO transaction is broken into chunk-sized blocks and loaded into STL objects. This method has a maximum memory requirement of `element_size * ( container_size + chunk_size )`

* `std::stack`,`std::queue`,`std::priority_queue` are adaptors; the underlying data-structure determines how data transfers take place.

In addition to the following mapping is considered:

* `vector | array`  of `vector | array` of ... -> `HDF5 ragged/fractal array`

#### :material-format-quote-open:{.icon} std::strings

HDF5 supports variable- and fixed-length strings. The former is of interest, as the most common way for storing strings in a file: consecutive characters with a separator. The current storage layout is a heap data structure making it less suitable for massive Terabyte-scale storage. In addition, the `std::string` s have to be copied during [read][read] operations. Both filtering, such as `h5::gzip{0-9}`, and encoding, such as `h5::utf8`, are supported.
<img src="../assets/c-strings-inmemory.png" alt="string"
   class="float-right w-2/5 border-2 border-solid border-red rounded-lg shadow-lg ml-4 transition-transform duration-500 hover:scale-110 bg-transparent"/>

#### C Strings
are actually one-dimensional array of characters terminated by a null character `\0`. Thus a null-terminated string contains the characters that comprise the string followed by a null. Am array of `char*` pointers is to describe a collection of c strings, the memory locations are not required to be
contiguous. The beginning of the array is a `char**` type.

`h5::write<char**>(fd, char** ptr, h5::current_dims{n_elems}, ...);`




**not supported**: `wchar_t _wchar char16_t _wchar16 char32_t _wchar32`





### Raw Pointers
Currently only memory blocks are supported in consecutive/adjacent location of elementary or POD types. This method comes in handy when an object type is not supported. You need to find a way to grab a pointer to its internal datastore and the size, and then pass this as an argument. For [read][read] operations, make sure there is enough memory reserved; for [write][write] operations, you must specify the data transfer size with `h5::count`.

**Example:** Let's load data from an HDF5 dataset to a memory location
```cpp
my_object obj(100);
h5::read("file.h5","dataset.dat",obj.ptr(), h5::count{10,10}, h5::offset{100,0});
```

### Compound Datatypes
#### POD Struct/Records
Arbitrarily deep and complex Plain Old Structured (POD) are supported either by the [h5cpp compiler][compiler] or by manually writing the necessary shim code. The following example was generated with the `h5cpp` compiler. Note that in the first step you have to specialize `template<class Type> hid_t inline register_struct<Type>();` to the type you want to use it with and return an HDF5 CAPI `hid_t` type identifier. This `hid_t` object references a memory location inside the HDF5 system, and will be automatically released with `H5Tclose` when used with H5CPP templates. The final step is to register this new type with H5CPP type system : `H5CPP_REGISTER_STRUCT(Type);`.

```cpp
namespace sn {
    struct PODstruct {
        ... 
        bool _bool;
    };
}
namespace h5{
    template<> hid_t inline register_struct<sn::PODstruct>(){
        hid_t ct_00 = H5Tcreate(H5T_COMPOUND, sizeof (sn::PODstruct));
        ...
        H5Tinsert(ct_00, "_bool",	HOFFSET(sn::PODstruct,_bool),H5T_NATIVE_HBOOL);
        return ct_00;
    };
}
H5CPP_REGISTER_STRUCT(sn::PODstruct);
```
The internal typesystem for POD/Record types supports:

* `std::is_integral` and `std::is_floating_point`
* `std::is_array` plain old array type, but not `std::array` or `std::vector` 
* arrays of the the above, with arbitrary nesting
* POD structs of the above with arbitrary nesting


#### C++ Classes
Work in progress. Requires modification to compiler as well as coordinated effort how to store complex objects such that other platforms capable of reading them.

# High Throughput Pipeline

HDF5 comes with a complex mechanism for type conversion, filtering, scatter/gather-funtions,etc. But what if you need to engineer a system to bare metal without frills? The `h5::high_throughput` data access property replaces the standard data processing mechanism with a BLAS level 3 blocking, a CPU cache-aware filter chain and delegates all calls to the `H5Dwrite_chunk` and `H5Dread_chunk` optimized calls.

**Example:** Let's save an `arma::mat M(16,32)` into an HDF5 data set using direct chunk write! First we pass the `h5::high_throughput` data access property when opening/creating data set and make certain to choose chunked layout 
by setting `h5::chunk{...}`. Optional standard filters and fill values may be set, however, the data set element type **must match**
the element type of `M`. No type conversion will be taking place!
```cpp
h5::ds_t ds = h5::create<double>(fd,"bare metal IO",
    h5::current_dims{43,57},     // doesn't have to be multiple of chunks
    h5::high_throughput,         // request IO pipeline
    h5::chunk{4,8} | h5::fill_value<double>{3} |  h5::gzip{9} );
```
You **must align all IO calls to chunk boundaries:** `h5::offset % h5::chunk == 0`. However, the dataset may have non-aligned size: `h5::count % h5::chunk != 0 -> OK`. Optionally, define the amount of data transferred with `h5::count{..}`. When `h5::count{...}` is not specified, the dimensions will be computed from the object. **Notice** `h5::offset{4,16}` is set to a chunk boundary.

The bahviour of saving data near edges matches the behaviour of standard CAPI IO calls. The chunk within an edge boundary having the correct content, and the outside being undefined.
```
h5::write( ds,  M, h5::count{4,8}, h5::offset{4,16} );
```
**Pros:**

* Fast IO with direct chunk write/read
* CPU cache aware filter chain
* Option for threaded + pipelined filter chain
* minimal code length, fast implementation

**Cons:**

* `h5::offset{..}` must be aligned with chunk boundaries
* `h5::stride`, `h5::block` and other sub setting methods such as scatter - gather will not work
* disk and memory type must match - no type conversion


The data set indeed is compressed, and readable from other systems:
```
HDF5 "arma.h5" {
GROUP "/" {
   DATASET "bare metal IO" {
      DATATYPE  H5T_IEEE_F64LE
      DATASPACE  SIMPLE { ( 40, 40 ) / ( 40, H5S_UNLIMITED ) }
      STORAGE_LAYOUT {
         CHUNKED ( 4, 8 )
         SIZE 79 (162.025:1 COMPRESSION)
      }
      FILTERS {
         COMPRESSION DEFLATE { LEVEL 9 }
      }
      FILLVALUE {
         FILL_TIME H5D_FILL_TIME_IFSET
         VALUE  3
      }
      ALLOCATION_TIME {
         H5D_ALLOC_TIME_INCR
      }
   }
}
}

```



[create]: index.md#create
[read]:   index.md#read
[write]:  index.md#write
[append]: index.md#append
[compiler]: compiler.md
[copy_elision]: https://en.cppreference.com/w/cpp/language/copy_elision
[csr]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_row_(CSR,_CRS_or_Yale_format)
[csc]: https://en.wikipedia.org/wiki/Sparse_matrix#Compressed_sparse_column_(CSC_or_CCS)
[not-implemented]: #not-implemented

[1]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_i.html
[2]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_i.html
[3]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_i.html

[11]: https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#S-errors
[12]: https://github.com/isocpp/CppCoreGuidelines/blob/master/CppCoreGuidelines.md#Re-exception-types
[13]: https://en.cppreference.com/w/cpp/language/namespace_alias
[14]: http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4436.pdf

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


[200]: https://en.wikipedia.org/wiki/Row-_and_column-major_order
[201]: https://en.wikipedia.org/wiki/Row-_and_column-major_order#Transposition
[300]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[301]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[302]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[303]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[304]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[305]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[306]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[307]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[308]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[309]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[400]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_d_o.html

[1000]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1001]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1002]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1003]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1004]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1005]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1006]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1007]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1008]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1009]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1010]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1020]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1021]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1022]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1023]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1024]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1025]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1026]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1027]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1028]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1029]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1030]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1031]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1032]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1033]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1034]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1035]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1036]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1037]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1038]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1039]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1040]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1041]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1042]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1043]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1044]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1045]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1046]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1047]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1048]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1049]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1050]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1051]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1052]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1053]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1054]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1055]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1100]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1101]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1102]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1103]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1104]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1200]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1201]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1300]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1301]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1302]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1400]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1401]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1402]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1403]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1404]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1405]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1406]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1500]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1501]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1502]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1503]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1504]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1505]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1506]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1507]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1508]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1509]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1510]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1511]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1512]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1513]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1600]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1601]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1602]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1603]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1604]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1605]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1700]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1701]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1702]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1703]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1704]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1705]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1706]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1707]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1708]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1709]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1710]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1711]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1712]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1713]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1800]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1801]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1802]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1803]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1804]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html

[1900]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1901]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html
[1902]: https://support.hdfgroup.org/documentation/hdf5/latest/group___h5_p.html


[601]: #file-creation-property-list
[602]: #file-access-property-list
[603]: #link-creation-property-list
[604]: #dataset-creation-property-list
[605]: #dataset-access-property-list
[606]: #dataset-transfer-property-list

[700]: http://man7.org/linux/man-pages/man2/fcntl.2.html
[701]: https://bitbucket.hdfgroup.org/users/jhenderson/repos/rest-vol/browse
