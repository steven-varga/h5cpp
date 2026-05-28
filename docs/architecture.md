---
hide:
  - toc
---

Within an HDF5 container, datasets may be stored using compact, chunked, or contiguous layouts. Objects are identified by slash-separated `(/)` paths. Non-leaf nodes are groups `(h5::gr_t)`, while terminal leaf nodes are datasets `(h5::ds_t)` and named types `(h5::dt_t)`. Groups, datasets, and named types may also carry attached attributes `(h5::att_t)`. At first glance, an HDF5 container resembles a regular file system, but with a richer and more specialized API.

#### :material-view-module:{.icon} Chunked Layout and Partial I/O
<img src="../assets/hdf5-dataset-layout-chunked.svg" alt="Chunked Layout"
   class="float-right w-2/5 border-2 border-solid border-red rounded-lg shadow-lg ml-4 transition-transform duration-500 hover:scale-110 bg-transparent"/>

Chunked layout is the standard way to work efficiently with large datasets when full-array I/O is unnecessary. Instead of storing data as one contiguous region, the dataset is divided into fixed-size chunks, which enables partial reads and writes and allows the use of filters such as compression. In H5CPP, chunked storage is requested by adding `h5::chunk{...}` to the dataset creation property list. This implicitly selects `h5::layout_chunked`.

```cpp
h5::ds_t ds = h5::create<double>(fd, "dataset", ...,
	h5::chunk{4, 8} | h5::fill_value<double>{3} | h5::gzip{9});
```

Here, `fd` is an open HDF5 file descriptor of type `h5::fd_t`, and the omitted arguments define the dataset extent. H5CPP supports practical partial I/O through hyperslab-style selections using `h5::offset{...}`, `h5::stride{...}`, and `h5::block{...}`. For example, let: `arma::mat M(20, 16);` To write `M` matrix into a larger dataset at a given offset, optionally with a stride, use: `h5::write(ds, M, h5::offset{4, 8}, h5::stride{2, 2});` H5CPP infers the memory address, datatype, and dimensions of supported objects and forwards them to the underlying HDF5 calls. When working with raw pointers, or with types not yet recognized by H5CPP, the logical shape must be specified explicitly with `h5::count{...}` as in `h5::write(ds, M.memptr(), h5::count{5, 10});` For common cases, dataset creation and full-object write can be expressed in a single call. The following creates a chunked dataset, applies a fill value and gzip compression, and writes the full contents of `M` matrix:

```cpp
h5::write(fd, "dataset", M, h5::chunk{4, 8} | h5::fill_value<double>{3} | h5::gzip{9});
```
See the [examples](examples.md) section for complete working cases.

#### :material-arrow-expand-horizontal:{.icon} Contiguous Layout and IO Access
<img src="../assets/hdf5-dataset-layout-contiguous.svg" alt="Contiguous Layout"
   class="float-left w-2/5 border-2 border-solid border-red rounded-lg shadow-lg mr-4 transition-transform duration-500 hover:scale-110 bg-transparent"/>

Contiguous layout is the simplest storage mode in HDF5. The dataset is stored as one continuous block on disk, which makes it a good fit when the entire object is typically written or read in a single operation. This layout works well for **small datasets** and for workloads that do not require compression, chunking, or partial I/O. It also keeps metadata overhead low, which can be beneficial when working with many small objects. When no filtering is requested and the object is written in one shot, H5CPP will generally choose this layout automatically.

**Example:** the following call opens `arma.h5`, creates a dataset with the appropriate dimensions and datatype, and writes the full content of the vector in a single operation.

```cpp
arma::vec V({1.,2.,3.,4.,5.,6.,7.,8.});
h5::write("arma.h5", "one shot create write", V);
```

To request contiguous layout explicitly, pass the `h5::contiguous` flag in the dataset creation property list.

The resulting dataset layout is conceptually equivalent to:

```text
DATASET "one shot create write" {
   DATATYPE  H5T_IEEE_F64LE
   DATASPACE  SIMPLE { ( 8 ) / ( 8 ) }
   STORAGE_LAYOUT {
      CONTIGUOUS
      SIZE 64
      OFFSET 5888
   }
   FILTERS {
      NONE
   }
   FILLVALUE {
      FILL_TIME H5D_FILL_TIME_IFSET
      VALUE  H5D_FILL_VALUE_DEFAULT
   }
   ALLOCATION_TIME {
      H5D_ALLOC_TIME_LATE
   }
}
```

The trade-off is straightforward: contiguous layout is simple and efficient for full-object I/O, but it does not support chunk-based filtering or efficient partial access. For small, dense datasets, that is often exactly the right choice.


#### :material-package-variant-closed:{.icon} Compact Layout
<img src="../assets/hdf5-dataset-layout-compact.svg" alt="Compact Layout"
   class="float-right w-2/5 border-2 border-solid border-red rounded-lg shadow-lg ml-4 transition-transform duration-500 hover:scale-110 bg-transparent"/>

Compact layout stores the entire dataset payload directly in the object header rather than in a separate data block. This minimizes indirection and can be very efficient for **small datasets** that are written and read as a whole. Because both metadata and data live together, access is simple and overhead is low. The trade-off is capacity and flexibility. Compact datasets are size-limited by the available object header space, so they are not suitable for larger arrays, append-style workflows, compression, or partial I/O. In practice, compact layout is most useful for very small fixed-size objects where minimizing storage overhead matters more than scalability. For anything expected to grow, be filtered, or accessed in pieces, chunked or contiguous layout is the better choice.


#### :material-ruler-square:{.icon} Dataspaces and Dimensions

A dataspace describes how data is shaped and mapped between memory and file storage. In practical terms, it tells HDF5 how an in-memory region corresponds to a dataset on disk, or how a region on disk should be read back into memory. For example, a contiguous block of memory may be interpreted as a vector, matrix, or cube-shaped dataset depending on the associated dataspace. A dataspace may have fixed extent, bounded extensible extent, or unlimited extent along one or more dimensions. When working with supported objects, H5CPP derives the in-memory dataspace automatically. When passing raw pointers to I/O operations, the relevant shape must be provided explicitly, and the file selection determines how much memory is transferred. The following descriptors define dataset dimensions:

* `h5::current_dims{i,j,k,...}` — current extent of the dataset
* `h5::max_dims{i,j,k,...}` — maximum extent; use `H5S_UNLIMITED` for an unbounded dimension
* `h5::chunk{i,j,k,...}` — chunk shape for chunked datasets; a suitable chunk layout can significantly improve performance

The following descriptors define read or write selections within a dataset:

* `h5::offset{i,j,k,...}` — starting coordinates of the selection
* `h5::stride{i,j,k,...}` — step between selected elements
* `h5::block{i,j,k,...}` — size of each selected block
* `h5::count{i,j,k,...}` — number of blocks to transfer

**Note:** `h5::stride`, `h5::block`, and scatter/gather-style selections are not available when `h5::high_throughput` is enabled, as that mode prioritizes simplified high-bandwidth transfer paths.

Here is a revised and restructured version in a cleaner reference style:

## :material-swap-horizontal:{.icon} **I/O Operators**

H5CPP uses C++ template metaprogramming to build a Pythonic I/O interface: calls are composed from strongly typed descriptors rather than rigid positional argument lists, while remaining fully resolved at compile time and incurring no runtime overhead. The following sections describe the core H5CPP I/O operators: `h5::create`, `h5::read`, `h5::write`, `h5::append`, `h5::open`, `h5::close` and their syntax is presented in EBNF-style notation. We begin with a few common terms. 

- **File or container** An HDF5 file may be viewed as a container with its own internal hierarchy of groups, datasets, named types, and attributes. The file itself is an ordinary file in the host file system and can be copied, moved, or archived using standard tools, while its contents are accessed through HDF5-specific I/O calls. To refer to such a container, pass either an open file handle `h5::fd_t`, a group handle `h5::gr_t`, or the path to the HDF5 file:

```cpp
file ::= const h5::gr_t | const h5::fd_t& fd | const std::string& file_path;
```

- **Groups and datasets** HDF5 groups (`h5::gr_t`) are similar to file-system directories: they act as containers for datasets, named types, and subgroups. For this reason, the terms *group* and *directory* are used interchangeably in this documentation. Like datasets, groups may also carry attributes. A dataset is a concrete object within the container. To identify one uniquely, pass either an open dataset handle `h5::ds_t`, or specify both the container and the dataset path. In the latter case, H5CPP generates the necessary shim code at compile time to obtain the corresponding `h5::fd_t` handle and resolve the dataset.

```cpp
dataset ::= (const h5::fd_t& fd, const std::string& dataset_path)
          | (const std::string& file_path, const std::string& dataset_path)
          | const h5::ds_t& ds;
```

- **Dataspace** HDF5 datasets may have different shapes in memory and on disk. A **dataspace** describes that shape: it specifies the current extent of the object and, where applicable, the maximum extent to which it may be extended.

```cpp
dataspace ::= const h5::sp_t& dataspace
            | const h5::current_dims& current_dims
            | const h5::current_dims& current_dims,
              const h5::max_dims& max_dims
            | const h5::max_dims& max_dims;
```

- **Type parameter `T`** denotes the template type parameter associated with the object being read or written. In H5CPP, the element type is deduced at compile time, which allows the library to support a flexible range of storage-capable C++ types. Broadly speaking, supported objects fall into two categories:

	* types backed by contiguous memory, such as vectors, matrices, and C-style POD or standard-layout records
	* more complex C++ object types with non-trivial internal structure

The latter are not generally supported as first-class storage objects. See the [Supported Types](#types) section for details.

#### :material-folder-open-outline:{.icon}  Open
The [previous section](#pythonic-syntax) introduced the common EBNF terms such as `file` and `dataspace`. Opening HDF5 objects follows the same general pattern: each object type has a corresponding open function, optionally parameterized with a property list.

```cpp
h5::fd_t h5::open( const std::string& path,  H5F_ACC_RDWR | H5F_ACC_RDONLY [, const h5::fapl_t& fapl] );
h5::gr_t h5::gopen( const h5::fd_t | h5::gr_t& location, const std::string& path [, const h5::gapl_t& gapl] );
h5::ds_t h5::open( const h5::fd_t | h5::gr_t& location, const std::string& path [, const h5::dapl_t& dapl] );
h5::at_t h5::aopen(const h5:ds_t | h5::gr_t& node, const std::string& name [, const & acpl] );
```

The optional property lists include [`h5::fapl_t`][602] for files and [`h5::dapl_t`][605] for datasets. These allow access behavior to be refined when needed. The supported file access flags are:

* `H5F_ACC_RDWR` — open with read/write access
* `H5F_ACC_RDONLY` — open with read-only access

In the standard HDF5 model, only one process may open a file for writing at a time, while multiple readers may access it concurrently. This is the usual single-writer / multiple-reader constraint. If exclusive write access is too restrictive, use **parallel HDF5**. In that model, multiple MPI processes may write to the same file using collective or independent access patterns such as `h5::collective` or `h5::independent`. On MPI-based clusters and supercomputers, parallel HDF5 is the preferred approach and can scale to very high aggregate throughput.

Here is a cleaner revision:

#### :material-plus-box-outline:{.icon} Create

Creation follows the same pattern as opening: each HDF5 object type has a corresponding create function, optionally parameterized with property lists.

```cpp
[file]
h5::fd_t h5::create( const std::string& path, H5F_ACC_TRUNC | H5F_ACC_EXCL, 
			[, const h5::fcpl_t& fcpl] [, const h5::fapl_t& fapl]);
[group]
h5::fd_t h5::gcreate( const h5::fd_t | const h5::gr_t, const std::string& name
			[, const h5::lcpl_t& lcpl] [, const h5::gcpl_t& gcpl] [, const h5::gapl_t& gapl]);
[dataset]
template <typename T> h5::ds_t h5::create<T>( 
	const h5::fd_t | const h5::gr_t& location, const std::string& dataset_path, dataspace, 
	[, const h5::lcpl_t& lcpl] [, const h5::dcpl_t& dcpl] [, const h5::dapl_t& dapl] );
[attribute]
template <typename T>
h5::at_t acreate<T>( const h5::ds_t | const h5::gr_t& | const h5::dt_t& node, const std::string& name
	 [, const h5::current_dims{...} ] [, const h5::acpl_t& acpl]);
```
The most commonly used property lists are: [`h5::fcpl_t`][601], [`h5::fapl_t`][602], [`h5::lcpl_t`][603], [`h5::dcpl_t`][604], and [`h5::dapl_t`][605]. The file creation flags are `H5F_ACC_TRUNC` — create the file, truncating it if it already exists, `H5F_ACC_EXCL` — create the file only if it does not already exist. The following creates an HDF5 file and then defines a chunked, extensible dataset.
```cpp
#include <h5cpp/all>
...
arma::mat M(2,3);
h5::fd_t fd = h5::create("arma.h5", H5F_ACC_TRUNC);
h5::ds_t ds = h5::create<short>(fd, "dataset/path/object.name",
    h5::current_dims{10, 20}, h5::max_dims{10, H5S_UNLIMITED}, h5::chunk{2, 3} | h5::fill_value<short>{3} | h5::gzip{9});
ds["attribute-name"] = std::vector<int>(10);
...
```

In this example:

* the file is created with truncation enabled
* the dataset is created with current dimensions `{10, 20}`
* the second dimension is declared extensible via `H5S_UNLIMITED`
* chunking, fill value, and gzip compression are specified through the dataset creation property list

A couple of technical notes.

* I changed `h5::fd_t` to `h5::gr_t` for `gcreate`, because returning a file descriptor there would be mildly alarming.
* `h5::at_t` may need to be `h5::att_t` if that is your actual attribute handle type elsewhere.
* `const h5::ds_t | const h5::gr_t | const h5::dt_t& node` is a bit uneven as written in the source; in real docs I would probably normalize all of those references visually.

The section is now much easier to scan: syntax first, property lists next, example after. Less soup, more structure.

Here is a cleaner revision of the section:

#### :material-database-arrow-down-outline:{.icon} Read

H5CPP provides two forms of read operators: one returns a **newly created object** for convenient one-shot access, while the other reads into **an existing object** for repeated use where avoiding allocation is important. When a value is returned, H5CPP creates the object with the appropriate shape and relies on return value optimization (RVO) so that no extra copy is introduced. When reading into an existing object, the caller is responsible for providing a correctly sized destination. H5CPP then uses the supplied memory directly whenever possible, avoiding unnecessary temporaries. Internally, HDF5 may still use a chunk-sized transfer buffer, for example for filtering or datatype conversion. In practice this overhead is usually small, and chunk sizes should generally be chosen so they remain cache-friendly.

```cpp
template <typename T> T h5::read( const h5::ds_t& ds
	[, const h5::offset_t& offset]  [, const h5::stride_t& stride] [, const h5::count_t& count]
	[, const h5::dxpl_t& dxpl ] ) const;
template <typename T> h5::err_t h5::read( const h5::ds_t& ds, T& ref 
	[, const [h5::offset_t& offset]  [, const h5::stride_t& stride] [, const h5::count_t& count]
	[, const h5::dxpl_t& dxpl ] ) const;

template <typename T> T aread( const h5::ds_t& | const h5::gr_t& | const h5::dt_t& node, 
	const std::string& name [, const h5::acpl_t& acpl]) const;
template <typename T> T aread( const h5::at_t& attr [, const h5::acpl_t& acpl]) const;
```

Dimensions of extent `1` are dropped automatically, so the result is returned as a 2D object. For repeated access inside a tight loop, read into a preallocated buffer:

```cpp
arma::mat buffer(3, 4);
for (int i = 0; i < 10'000'000; i++)
    h5::read(ds, buffer, h5::offset{i + chunk});
```

In this form, `h5::count{...}` is inferred from the size of `buffer`. Passing an explicit count that does not match the target object can produce a useful compile-time diagnostic. When working with raw or managed pointers, the target memory extent must be specified explicitly:

```cpp
std::unique_ptr<double[]> ptr(new double[buffer_size]);

for (int i = 0; i < 10'000'000; i++)
    h5::read(ds, ptr.get(), h5::offset{i + buffer_size}, h5::count{buffer_size});
```

#### :material-database-arrow-up-outline:{.icon} WRITE
H5CPP provides two forms of write operators. **Reference-based writes** are intended for object types the library understands directly, such as supported containers, matrices, and records, and **are generally the preferred** interface because they combine convenience with the same performance characteristics as pointer-based access. **Pointer-based writes** operate on arbitrary contiguous memory regions and **are** mainly **useful in cases where no higher-level type mapping is available**; in that case, you provide the memory address and transfer properties, and H5CPP handles the rest. As with reads, the underlying HDF5 library may still allocate a chunk-sized transfer buffer, typically for filtering or datatype conversion, but this overhead is usually small when chunk sizes are chosen sensibly—ideally no larger than the processor’s last-level cache.

```cpp
[dataset]
template <typename T> void h5::write( dataset,  const T& ref
	[,const h5::offset_t& offset] [,const h5::stride_t& stride]  [,const& h5::dxcpl_t& dxpl] );
template <typename T> void h5::write( dataset, const T* ptr
	[,const hsize_t* offset] [,const hsize_t* stride] ,const hsize_t* count [, const h5::dxpl_t dxpl ]);

[attribute]
template <typename T> void awrite( const h5::ds_t& | const h5::gr_t& | const h5::dt_t& node, 
	const std::string &name, const T& obj  [, const h5::acpl_t& acpl]);
template <typename T> void awrite( const h5::at_t& attr, const T* ptr [, const h5::acpl_t& acpl]);

```
Property lists are:  [`dxpl_t`][606]

```cpp
#include <Eigen/Dense>
#include <h5cpp/all>

h5::fd_t fd = h5::create("some_file.h5",H5F_ACC_TRUNC);
h5::write(fd,"/result",M);
```

####  :material-table-row-plus-after:{.icon} Append
For streaming or append-only workloads, **packet tables are** often the most **practical** choice. Although the append operator uses its own `h5::pt_t` handle, the underlying storage object is still an ordinary HDF5 dataset, and conversion between `h5::pt_t` and `h5::ds_t` is seamless. The main difference is internal: unlike the other H5CPP handles, `h5::pt_t` maintains its own buffer and uses a specialized transfer pipeline optimized for incremental writes. The same pipeline may also be enabled for regular dataset I/O through the `h5::experimental` data-transfer property, as described in the [experimental pipeline documentation][308].

``` c++
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
template <typename T> void h5::append(h5::pt_t& ds, const T& ref);
```

**Example:**
``` c++
#include <h5cpp/core>
	#include "your_data_definition.h"
#include <h5cpp/io>
auto fd = h5::create("NYSE high freq dataset.h5");
h5::pt_t pt = h5::create<ns::nyse_stock_quote>( fd, 
		"price_quotes/2018-01-05.qte",h5::max_dims{H5S_UNLIMITED}, h5::chunk{1024} | h5::gzip{9} );
quote_update_t qu;

bool having_a_good_day{true};
while( having_a_good_day ){
	try{
		recieve_data_from_udp_stream( qu )
		h5::append(pt, qu);
	} catch ( ... ){
	  if( cant_fix_connection() )
	  		having_a_good_day = false; 
	}
}
```

#  :material-tag-multiple-outline:{.icon} Attributes

Attributes are the standard HDF5 mechanism for attaching side-band metadata to groups (`h5::gr_t`), datasets (`h5::ds_t`), and named datatypes (`h5::dt_t`). In H5CPP, the same core I/O model applies here as well, so attributes support the same family of storage-capable object types described in the [Supported Types](#supported-objects) section. In the definitions below, `P ::= h5::ds_t | h5::gr_t | h5::ob_t | h5::dt_t` denotes any valid HDF5 object handle, while `std::tuple<T...>` represents a sequence of pairwise attribute operations, where consecutive tuple elements are interpreted as attribute-name / attribute-value pairs.

```cpp
h5::at_t acreate( const P& parent, const std::string& name, args_t&&... args );
h5::at_t aopen(const  P& parent, const std::string& name, const h5::acpl_t& acpl);
T aread( const P& parent, const std::string& name, const h5::acpl_t& acpl)
h5::att_t awrite( const P& parent, const std::string& name, const T& ref, const h5::acpl_t& acpl)
void awrite( const P& parent, const std::tuple<Field...>& fields, const h5::acpl_t& acpl)
```

**Example:** the following snippet creates three attributes and attaches them to an `h5::gr_t` group. It also illustrates mixed use of the HDF5 C API and H5CPP, including how a raw `hid_t` handle returned by the C API may be wrapped in the RAII-enabled `h5::gr_t` handle.
```cpp
auto attributes = std::make_tuple(
		"author", "steven varga",  "company","vargaconsulting.ca",  "year", 2019);
// freely mixed CAPI call returned `hid_t id` is wrapped in RAII capable H5CPP template class: 
h5::gr_t group{H5Gopen(fd,"/my-data-set", H5P_DEFAULT)};
// templates will synthesize code from std::tuple, resulting in 3 attributes created and written into `group` 
h5::awrite(group, attributes);
```

# :material-shape-outline:{.icon} Supported Objects

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

Here is a slightly cleaner, more book-like version:

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

**TODO:** work out a new efficient storage mechanism for strings.




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

# MPI-IO
Parallel Filesystems 

# Type System
At the heart of H5CPP lies the type mapping mechanism to HDF5 NATIVE types. All type requests are redirected to this segment in one way or another. That includes supported vectors, matrices, cubes, C like structs, etc. While HDF5 internally supports type translations among various binary representation, H5CPP deals only with native types. This is not a violation of the HDF5 use-anywhere policy, only type conversion is delegated to hosts with different binary representations. Since the most common processors are Intel and AMD, with this approach conversion is unnescessary most of the time. In summary, H5CPP uses NAIVE types exclusively.

```yacc
integral 		:= [ unsigned | signed ] [int_8 | int_16 | int_32 | int_64 | float | double ] 
vectors  		:=  *integral
rugged_arrays 	:= **integral
string 			:= **char
linalg 			:= armadillo | eigen | ... 
scalar 			:= integral | pod_struct | string

# not handled yet: long double, complex, specialty types
```

Here is the relevant part responsible for type mapping:
```cpp
#define H5CPP_REGISTER_TYPE_( C_TYPE, H5_TYPE )                                           \
namespace h5 { namespace impl { namespace detail { 	                                      \
	template <> struct hid_t<C_TYPE,H5Tclose,true,true,hdf5::type> : public dt_p<C_TYPE> {\
		using parent = dt_p<C_TYPE>;                                                      \
		using parent::hid_t;                                                              \
		using hidtype = C_TYPE;                                                           \
		hid_t() : parent( H5Tcopy( H5_TYPE ) ) { 										  \
			hid_t id = static_cast<hid_t>( *this );                                       \
			if constexpr ( std::is_pointer<C_TYPE>::value )                               \
					H5Tset_size (id,H5T_VARIABLE), H5Tset_cset(id, H5T_CSET_UTF8);        \
		}                                                                                 \
	};                                                                                    \
}}}                                                                                       \
namespace h5 {                                                                            \
	template <> struct name<C_TYPE> {                                                     \
		static constexpr char const * value = #C_TYPE;                                    \
	};                                                                                    \
}                                                                                         \
```
Arithmetic types are associated with their NATIVE HDF5 equivalent:
```cpp
H5CPP_REGISTER_TYPE_(bool,H5T_NATIVE_HBOOL)

H5CPP_REGISTER_TYPE_(unsigned char, H5T_NATIVE_UCHAR) 			H5CPP_REGISTER_TYPE_(char, H5T_NATIVE_CHAR)
H5CPP_REGISTER_TYPE_(unsigned short, H5T_NATIVE_USHORT) 		H5CPP_REGISTER_TYPE_(short, H5T_NATIVE_SHORT)
H5CPP_REGISTER_TYPE_(unsigned int, H5T_NATIVE_UINT) 			H5CPP_REGISTER_TYPE_(int, H5T_NATIVE_INT)
H5CPP_REGISTER_TYPE_(unsigned long int, H5T_NATIVE_ULONG) 		H5CPP_REGISTER_TYPE_(long int, H5T_NATIVE_LONG)
H5CPP_REGISTER_TYPE_(unsigned long long int, H5T_NATIVE_ULLONG) H5CPP_REGISTER_TYPE_(long long int, H5T_NATIVE_LLONG)
H5CPP_REGISTER_TYPE_(float, H5T_NATIVE_FLOAT) 					H5CPP_REGISTER_TYPE_(double, H5T_NATIVE_DOUBLE)
H5CPP_REGISTER_TYPE_(long double,H5T_NATIVE_LDOUBLE)

H5CPP_REGISTER_TYPE_(char*, H5T_C_S1)
```
Record/POD struct types are registered through this macro:
```cpp
#define H5CPP_REGISTER_STRUCT( POD_STRUCT ) \
	H5CPP_REGISTER_TYPE_( POD_STRUCT, h5::register_struct<POD_STRUCT>() )
```
**FYI:** there are no other public/unregistered macros other than `H5CPP_REGISTER_STRUCT`

### Resource Handles and CAPI Interop
By default the `hid_t` type is automatically converted to / from H5CPP `h5::hid_t<T>` templated identifiers. All HDF5 CAPI identifiers are wrapped via the `h5::impl::hid_t<T>` internal template, maintaining binary compatibility, with the exception of `h5::pt_t` packet table handle.


Here are some properties of descriptors,`h5::ds_t` and `h5::open` are used only for the demonstration, replace them with arbitrary descriptors.

* `h5::ds_t ds` default CTOR, is initialized with `H5I_UNINIT`
* `h5::ds_t res = h5::open(...)` RVO  with RAII maneged resource `H5Iinc_ref` not called, best way to initialize
* `h5::ds_t res = std::move(h5::ds_t(...))` move assignment with RAII enabled for each handles, `H5Iinc_ref` called once
* `h5::ds_t res = h5::ds_t(...)` copy assignment with RAII enabled for each handles, `H5Iinc_ref` called twice, considered inexpensive
* `h5::ds_t res(hid_t)` creates resource with RAII, increments reference counts of `hid_t` with `H5Iinc_ref` 
* `h5::ds_t res{hid_t}` create resource without RAII, for managed CAPI handles, try not using it, a full copy of a handle is cheap

The Packet Table interface has an additional conversion CTOR defined from `h5::ds_t` to `h5::pt_t` and is the preffered way to obtain a handle:

* `h5::pt_t pt = h5::open(fd, 'path/to/dataset', ...)` obtains a `h5::ds_t` resource, converts it to `h5::pt_t` type then assigns it to names variable, this process will result in 2 increments to underlying CAPI handle or 2 calls to `H5Iinc_ref`. 
*  `h5::pt_t pt = h5::ds_t(...)` this pattern is used when creating packet table from existing and opened `h5::ds_t`, resources are copied (not stolen), in other words `h5::ds_t` remains valid after the operation, all state changes will be reflected.

During the lifespan of H5CPP handles all of the underlying HDF5 desciptors are **properly initialized, reference count maintained and closed.**

Resources may be grouped into `handles` and `property_lists`-s. 
```yacc
T := [ handles | property_list ]
handles   := [ fd_t | ds_t | att_t | err_t | grp_t | id_t | obj_t ]
property_lists := [ file | dataset | attrib | group | link | string | type | object ]

#            create       access       transfer     copy 
file    := [ h5::fcpl_t | h5::fapl_t                            ] 
dataset := [ h5::dcpl_t | h5::dapl_t | h5::dxpl_t               ]
attrib  := [ h5::acpl_t                                         ] 
group   := [ h5::gcpl_t | h5::gapl_t                            ]
link    := [ h5::lcpl_t | h5::lapl_t                            ]
string  := [              h5::scpl_t                            ] 
type    := [              h5::tapl_t                            ]
object  := [ h5::ocpl_t                           | h5::ocpyl_t ]
```

# Property Lists

The functions, macros, and subroutines listed here are used to manipulate property list objects in various ways, including to reset property values. With the use of property lists, HDF5 functions have been implemented and can be used in applications with fewer parameters than would be required without property lists, this mechanism is similar to [POSIX fcntl][700]. Properties are grouped into classes, and each class member may be daisy chained to obtain a property list.

Here is an example of how to obtain a data creation property list with chunk, fill value, shuffling, nbit, fletcher32 filters and gzip compression set:
```cpp
h5::dcpl_t dcpl = h5::chunk{2,3} 
	| h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
auto ds = h5::create("container.h5","/my/dataset.dat", h5::create_path | h5::utf8, dcpl, h5::default_dapl);
```
Properties may be passed in arbitrary order, by reference, or directly by daisy chaining them. The following property list descriptors are available:
```yacc
#            create       access       transfer     copy 
file    := [ h5::fcpl_t | h5::fapl_t                            ] 
dataset := [ h5::dcpl_t | h5::dapl_t | h5::dxpl_t               ]
attrib  := [ h5::acpl_t                                         ] 
group   := [ h5::gcpl_t | h5::gapl_t                            ]
link    := [ h5::lcpl_t | h5::lapl_t                            ]
string  := [              h5::scpl_t                            ] 
type    := [              h5::tapl_t                            ]
object  := [ h5::ocpl_t                           | h5::ocpyl_t ]
```

#### Default Properties:
Set to a non-default value (different from HDF5 CAPI):

* `h5::default_lcpl =  h5::utf8 | h5::create_intermediate_group;`

Reset to default (same as HDF5 CAPI):

* `h5::default_acpl`, `h5::default_dcpl`, `h5::default_dxpl`, `h5::default_fapl`,  `h5::default_fcpl`


## File Operations
#### [File Creation Property List][1001]
```cpp
// you may pass CAPI property list descriptors whose properties are daisy chained with the '|' operator 
auto fd = h5::create("002.h5", H5F_ACC_TRUNC, 
		h5::file_space_page_size{4096} | h5::userblock{512},  // file creation properties
		h5::fclose_degree_weak | h5::fapl_core{2048,1} );     // file access properties
```

* [`h5::userblock{hsize_t}`][1001] Sets the user block size of a file creation property list
* [`h5::sizes{size_t,size_t}`][1002] Sets the byte size of the offsets and lengths used to address objects in an HDF5 file.
* [`h5::sym_k{unsigned,unsigned}`][1003] Sets the size of parameters used to control the symbol table nodes.
* [`h5::istore_k{unsigned}`][1004] Sets the size of the parameter used to control the B-trees for indexing chunked datasets.
* [`h5::file_space_page_size{hsize_t}`][1005] Sets the file space page size for a file creation property list.
* [`h5::file_space_page_strategy{H5F_fspace_strategy_t strategy, hbool_t persist, hsize_t threshold}`][1010] Sets the file space handling strategy and persisting free-space values for a file creation property list. <br/>
* [`h5::shared_mesg_nindexes{unsigned}`][1007] Sets number of shared object header message indexes.
* [`h5::shared_mesg_index{unsigned,unsigned,unsigned}`][1008] Configures the specified shared object header message index.
* [`h5::shared_mesg_phase_change{unsigned,unsigned}`][1009] Sets shared object header message storage phase change thresholds.

#### [File Access Property List][1020]
**Example:**
```cpp
h5::fapl_t fapl = h5::fclose_degree_weak | h5::fapl_core{2048,1} | h5::core_write_tracking{false,1} 
			| h5::fapl_family{H5F_FAMILY_DEFAULT,0};
			
```
* [`h5::driver{hid_t new_driver_id, const void *new_driver_info}`][1055] Sets a file driver.<br/>
* [`h5::fclose_degree{H5F_close_degree_t}`][1022] Sets the file close degree.<br/>
	**Flags:** `h5::fclose_degree_weak`, `h5::fclose_degree_semi`, `h5::fclose_degree_strong`, `h5::fclose_degree_default`
* [`h5::fapl_core{size_t increment, hbool_t backing_store}`][1023] Modifies the file access property list to use the H5FD_CORE driver.
* [`h5::core_write_tracking{hbool_t is_enabled, size_t page_size}`][1024] Sets write tracking information for core driver, H5FD_CORE. 
* [`h5::fapl_direct{size_t alignment, size_t block_size, size_t cbuf_size}`][1025] Sets up use of the direct I/O driver.
* [`h5::fapl_family{hsize_t memb_size, hid_t memb_fapl_id}`][1026] Sets the file access property list to use the family driver.
* [`h5::family_offset{hsize_t offset}`][1027] Sets offset property for low-level access to a file in a family of files.
* [`h5::fapl_log{const char *logfile, unsigned long long flags, size_t buf_size}`][1028] Sets up the logging virtual file driver (H5FD_LOG) for use.
* [`h5::fapl_mpiio{MPI_Comm comm, MPI_Info info}`][1029] Stores MPI IO communicator information to the file access property list.
* [`h5::multi{const H5FD_mem_t *memb_map, const hid_t *memb_fapl, const char * const *memb_name, const haddr_t *memb_addr, hbool_t relax}`][1030] Sets up use of the multi-file driver.
* [`h5::multi_type{H5FD_mem_t}`][1031] Specifies type of data to be accessed via the MULTI driver, enabling more direct access. <br>
**Flags:** `h5::multi_type_super`, `h5::multi_type_btree`, `h5::multi_type_draw`, `h5::multi_type_gheap`, `h5::multi_type_lheap`, `h5::multi_type_ohdr`
* [`h5::fapl_split{const char *meta_ext, hid_t meta_plist_id, const char *raw_ext, hid_t raw_plist_id}`][1032] Emulates the old split file driver.
* [`h5::sec2`][1033] (flag) Sets the sec2 driver.
* [`h5::stdio`][1034] (flag) Sets the standard I/O driver.
* [`h5::windows`][1035] (flag) Sets the Windows I/O driver.
* [`h5::file_image{void*,size_t}`][1036] Sets an initial file image in a memory buffer.
* [`h5::file_image_callback{H5_file_image_callbacks_t *callbacks_ptr}`][1037] Sets the callbacks for working with file images.
* [`h5::meta_block_size{hsize_t}`][1038] Sets the minimum metadata block size.
* [`h5::page_buffer_size{size_t,unsigned,unsigned}`][1039] Sets the maximum size for the page buffer and the minimum percentage for metadata and raw data pages.
* [`h5::sieve_buf_size{size_t}`][1040] Sets the maximum size of the data sieve buffer.
* [`h5::alignment{hsize_t, hsize_t}`][1041] Sets alignment properties of a file access property list.
* [`h5::cache{int,size_t,size_t,double}`][1042] Sets the raw data chunk cache parameters.
* [`h5::elink_file_cache_size{unsigned}`][1043] Sets the number of files that can be held open in an external link open file cache.
* [`h5::evict_on_close{hbool_t}`][1044] Controls the library's behavior of evicting metadata associated with a closed object
* [`h5::metadata_read_attempts{unsigned}`][1045] Sets the number of read attempts in a file access property list.
* [`h5::mdc_config{H5AC_cache_config_t*}`][1046] Set the initial metadata cache configuration in the indicated File Access Property List to the supplied value.
* [`h5::mdc_image_config{H5AC_cache_image_config_t * config_ptr}`][1047] Sets the metadata cache image option for a file access property list.
* [`h5::mdc_log_options{const char*,hbool_t}`][1048] Sets metadata cache logging options.
* [`h5::all_coll_metadata_ops{hbool_t}`][1049] Sets metadata I/O mode for read operations to collective or independent (default).
* [`h5::coll_metadata_write{hbool_t}`][1050] Sets metadata write mode to collective or independent (default).
* [`h5::gc_references{unsigned}`][1050] H5Pset_gc_references sets the flag for garbage collecting references for the file.
* [`h5::small_data_block_size{hsize_t}`][1052] Sets the size of a contiguous block reserved for small data.
* [`h5::libver_bounds {H5F_libver_t,H5F_libver_t}`][1053] Sets bounds on library versions, and indirectly format versions, to be used when creating objects.
* [`h5::object_flush_cb{H5F_flush_cb_t,void*}`][1054] Sets a callback function to invoke when an object flush occurs in the file.
* **`h5::fapl_rest_vol`** or to request KITA/RestVOL services both flags are interchangeable you only need to specify one of them follow [instructions:][701] to setup RestVOL, once the required modules are included
* **`h5::kita`** same as above

## Group Operations
#### [Group Creation Property List][1100]

* [`h5::local_heap_size_hint{size_t}`][1101] Specifies the anticipated maximum size of a local heap.
* [`h5::link_creation_order{unsigned}`][1102] Sets creation order tracking and indexing for links in a group.
* [`h5::est_link_info{unsigned, unsigned}`][1103] Sets estimated number of links and length of link names in a group.
* [`h5::link_phase_change{unsigned, unsigned}`][1104] Sets the parameters for conversion between compact and dense groups. 

#### [Group Access Property List][1200]
* [`local_heap_size_hint{hbool_t is_collective}`][1201] Sets metadata I/O mode for read operations to collective or independent (default).


## Link Operations
#### [Link Creation Property List][1300]
* [`h5::char_encoding{H5T_cset_t}`][1301] Sets the character encoding used to encode link and attribute names. <br/>
**Flags:** `h5::utf8`, `h5::ascii`
* [`h5::create_intermediate_group{unsigned}`][1302] Specifies in property list whether to create missing intermediate groups. <br/>
**Flags** `h5::create_path`, `h5::dont_create_path`

#### [Link Access Property List][1400]
* [`h5::nlinks{size_t}`][1401] Sets maximum number of soft or user-defined link traversals.
* [`h5::elink_cb{H5L_elink_traverse_t, void*}`][1402] Sets metadata I/O mode for read operations to collective or independent (default).
* [`h5::elink_fapl{hid_t}`][1403] Sets a file access property list for use in accessing a file pointed to by an external link.
* [`h5::elink_acc_flags{unsigned}`][1403] Sets the external link traversal file access flag in a link access property list.<br/>
	**Flags:** 	`h5::acc_rdwr`, `h5::acc_rdonly`, `h5::acc_default`

## Dataset Operations
#### [Dataset Creation Property List][1500]
Filtering properties require `h5::chunk{..}` set to sensible values. Not having set `h5::chunk{..}` set is equal with requesting `h5::layout_contigous` a dataset layout without chunks, filtering and sub-setting capabilities. This layout is useful for single shot read/write operations, and is the prefered method to save small linear algebra objects.
**Example:**
```cpp
h5::dcpl_t dcpl = h5::chunk{1,4,5} | h5::deflate{4} | h5::compact | h5::dont_filter_partial_chunks
		| h5::fill_value<my_struct>{STR} | h5::fill_time_never | h5::alloc_time_early 
		| h5::fletcher32 | h5::shuffle | h5::nbit;
```
* [`h5::layout{H5D_layout_t layout}`][1501] Sets the type of storage used to store the raw data for a dataset. <br/>
**Flags:** `h5::compact`, `h5::contigous`, `h5::chunked`, `h5::virtual`
* [`h5::chunk{...}`][1502] control chunk size, takes in initializer list with rank matching the dataset dimensions, sets `h5::chunked` layout
* [`h5::chunk_opts{unsigned}`][1503] Sets the edge chunk option in a dataset creation property list.<br/>
**Flags:** `h5::dont_filter_partial_chunks`
* [`h5::deflate{0-9}`][1504] | `h5::gzip{0-9}` set deflate compression ratio
* [`h5::fill_value<T>{T* ptr}`][1505] sets fill value
* [`h5::fill_time{H5D_fill_time_t fill_time}`][1506] Sets the time when fill values are written to a dataset.<br/>
**Flags:** `h5::fill_time_ifset`, `h5::fill_time_alloc`, `h5::fill_time_never`
* [`h5::alloc_time{H5D_alloc_time_t alloc_time}`][1507] Sets the timing for storage space allocation.<br/>
**Flags:** `h5::alloc_time_default`, `h5::alloc_time_early`, `h5::alloc_time_incr`, `h5::alloc_time_late`
* [`h5::fletcher32`][1509] Sets up use of the Fletcher32 checksum filter.
* [`h5::nbit`][1510] Sets up the use of the N-Bit filter.
* [`h5::shuffle`][1512] Sets up use of the shuffle filter.

#### [Dataset Access Property List][1600]
In addition to CAPI properties the follwoing properties are added to provide finer control dataset layouts, and filter chains.

* Mutually exclusive Sparse Matrix Properties to alternate between formats

	* **`h5::csc`** compressed sparse column format
	* **`h5::csr`** compressed sparse row format
	* **`h5::coo`** coordinates list

* **`h5::multi{["field 0", .., "field n"]}`** objects are persisted in multiple datasets with optional field name definition. When specified the system will break up compound classes into basic components and writes each of them into a directory/folder specified with `dataset_name` argument.

* **`h5::high_throughput`** Sets high throughput H5CPP custom filter chain. HDF5 library comes with a complex, feature rich environment to index data-sets by strides, blocks, or even by individual coordinates within chunk boundaries - less fortunate the performance impact on throughput.  Setting this flag will replace the built in filter chain with a simpler one (without complex indexing features), then delegates IO calls to the recently introduced [HDF5 Optimized API][400] calls.<br/>

	The implementation is based on BLAS level 3 blocking algorithm, supports data access only at chunk boundaries, edges are handled as expected. For maximum throughput place edges at chunk boundaries.
	**Note:** This feature and indexing within a chunk boundary such as `h5::stride` is mutually exclusive.

* [`h5::chunk_cache{size_t rdcc_nslots, size_t rdcc_nbytes, double rdcc_w0}`][1601] Sets the raw data chunk cache parameters.
* [`all_coll_metadata_ops{hbool_t is_collective}`][1602] Sets metadata I/O mode for read operations to collective or independent (default).
* [`h5::efile_prefix{const char*}`][1603] Sets the external dataset storage file prefix in the dataset access property list.
* [*`h5::append_flush`*][not-implemented] N/A, H5CPP comes with custom high performance packet table implementation
* [`h5::virtual_view{H5D_vds_view_t}`][1604] Sets the view of the virtual dataset (VDS) to include or exclude missing mapped elements.
* [`h5::virtual_printf_gap{hsize_t}`][1605] Sets the maximum number of missing source files and/or datasets with the printf-style names when getting the extent of an unlimited virtual dataset.

#### [Dataset Transfer Property List][1700]

* [`h5::buffer{size_t, void*, void*}`][1701] Sets type conversion and background buffers.
* [`h5::edc_check{H5Z_EDC_t}`][1702] Sets whether to enable error-detection when reading a dataset.
* [`h5::filter_callback{H5Z_filter_func_t func, void *op_data}`][1703] Sets user-defined filter callback function.
* [`h5::data_transform{const char *expression}`][1704] Sets a data transform expression.
* [`h5::type_conv_cb{H5T_conv_except_func_t func, void *op_data}`][1705] Sets user-defined datatype conversion callback function.
* [`h5::hyper_vector_size{size_t vector_size}`][1706] Sets number of I/O vectors to be read/written in hyperslab I/O.
* [`h5::btree_ratios{double left, double middle, double right}`][1707] Sets B-tree split ratios for a dataset transfer property list.
* [`h5::vlen_mem_manager{H5MM_allocate_t alloc, void *alloc_info, H5MM_free_t free, void *free_info}`][1708]
* [`h5::dxpl_mpiio{H5FD_mpio_xfer_t xfer_mode}`][1709] Sets data transfer mode <br/>
**Flags:** `h5::collective`, `h5::independent`
* [`h5::dxpl_mpiio_chunk_opt{H5FD_mpio_chunk_opt_t opt_mode}`][1710] Sets a flag specifying linked-chunk I/O or multi-chunk I/O.<br/>
**Flags:** `h5::chunk_one_io`, `h5::chunk_multi_io`
* [`h5::dxpl_mpiio_chunk_opt_num{unsigned num_chunk_per_proc}`][1711] Sets a numeric threshold for linked-chunk I/O.
* [`h5::dxpl_mpiio_chunk_opt_ratio{unsigned percent_proc_per_chunk}`][1712] Sets a ratio threshold for collective I/O.
* [`h5::dxpl_mpiio_collective_opt{H5FD_mpio_collective_opt_t opt_mode}`][1713] Sets a flag governing the use of independent versus collective I/O.<br/>
**Flags:** `h5::collective_io`, `h5::individual_io`

## Misc Operations
#### [Object Creation Property List][1800]
* [`h5::ocreate_intermediate_group{unsigned}`][1801] Sets tracking and indexing of attribute creation order.
* [`h5::obj_track_times{hbool_t}`][1802] Sets the recording of times associated with an object.
* [`h5::attr_phase_change{unsigned, unsigned}`][1803] Sets attribute storage phase change thresholds.
* [`h5::attr_creation_order{unsigned}`][1804] Sets tracking and indexing of attribute creation order. <br/>
**Flags:** `h5::crt_order_tracked`, `h5::crt_order_indexed`

#### [Object Copy Property List][1900]
* [`h5::copy_object{unsigned}`][1901] Sets properties to be used when an object is copied. <br/>
**Flags:** `h5::shallow_hierarchy`, `h5::expand_soft_link`, `h5::expand_ext_link`, `h5::expand_reference`, `h5::copy_without_attr`,
`h5::merge_commited_dtype`

## MPI / Parallel Operations
* [`h5::fapl_mpiio{MPI_Comm comm, MPI_Info info}`][1029] Stores MPI IO communicator information to the file access property list.
* [`h5::all_coll_metadata_ops{hbool_t}`][1049] Sets metadata I/O mode for read operations to collective or independent (default).
* [`h5::coll_metadata_write{hbool_t}`][1050] Sets metadata write mode to collective or independent (default).
* [`h5::gc_references{unsigned}`][1051] H5Pset_gc_references sets the flag for garbage collecting references for the file.
* [`h5::small_data_block_size{hsize_t}`][1052] Sets the size of a contiguous block reserved for small data.
* [`h5::object_flush_cb{H5F_flush_cb_t,void*}`][1054] Sets a callback function to invoke when an object flush occurs in the file.
* [`h5::fapl_coll_metadata_ops{hbool_t}`] 
* [`h5::gapl_coll_metadata_ops{hbool_t}`] 
* [`h5::dapl_coll_metadata_ops{hbool_t}`] 
* [`h5::tapl_coll_metadata_ops{hbool_t}`] 
* [`h5::lapl_coll_metadata_ops{hbool_t}`] 
* [`h5::aapl_coll_metadata_ops{hbool_t}`] 
* [`h5::dxpl_mpiio{H5FD_mpio_xfer_t xfer_mode}`][1709] Sets data transfer mode <br/>
**Flags:** `h5::collective`, `h5::independent`
* [`h5::dxpl_mpiio_chunk_opt{H5FD_mpio_chunk_opt_t opt_mode}`][1710] Sets a flag specifying linked-chunk I/O or multi-chunk I/O.<br/>
**Flags:** `h5::chunk_one_io`, `h5::chunk_multi_io`
* [`h5::dxpl_mpiio_chunk_opt_num{unsigned num_chunk_per_proc}`][1711] Sets a numeric threshold for linked-chunk I/O.
* [`h5::dxpl_mpiio_chunk_opt_ratio{unsigned percent_proc_per_chunk}`][1712] Sets a ratio threshold for collective I/O.
* [`h5::dxpl_mpiio_collective_opt{H5FD_mpio_collective_opt_t opt_mode}`][1713] Sets a flag governing the use of independent versus collective I/O.<br/>
**Flags:** `h5::collective_io`, `h5::individual_io`



## C++ Idioms


### RAII 

There is a C++ mapping for `hid_t` id-s, which reference objects, with `std::shared_ptr` type of behaviour with HDF5 CAPI internal reference counting.

 For further details see [H5Iinc_ref][1], [H5Idec_ref][2] and [H5Iget_ref][3]. The internal representation of these objects is binary compatible with the CAPI `hid_t` and interchangeable depending on the conversion policy:
	`H5_some_function( static_cast<hid_t>( h5::hid_t id ), ...   )`
Direct initialization `h5::ds_t{ some_hid }` bypasses reference counting, and is intended for use cases where you have to take ownership
of an CAPI `hid_t` object handle. This is equivalent behaviour to `std::shared_ptr`, where a referenced object is destroyed when its reference count reaches 0.
```cpp
{
	h5::ds_t ds = h5::open( ... ); 
} // resources are guaranteed to be released
```

### Error handling 

Error handling follows the C++ [Guidline][11] and the H5CPP "philosophy" which is to  help you to get started without reading a lot of documentation, and to provide ample room for more should you require it. The root of the exception tree is: `h5::error::any` derived from std::`runtime_exception` in accordance with the C++ guidelines [custom exceptions][12]. 
All HDF5 CAPI calls are considered resources, and, in case of an error, H5CPP aims to roll back to the last known stable state while cleaning up all resource allocations between the call entry and the thrown error. This mechanism is guaranteed by RAII. 

For granularity `io::[file|dataset|attribute]` exceptions are provided, with the pattern to capture the entire subset by `::any`.
Exceptions are thrown with error messages, \__FILE\__ and \__LINE\__ relevant to H5CPP template library, with a brief description to help the developer to investigate. This error reporting mechanism uses a macro found inside **h5cpp/config.h** and maybe redefined:
```cpp
	...
// redefine macro before including <h5cpp/ ... >
#define H5CPP_ERROR_MSG( msg ) "MY_ERROR: " 
	+ std::string( __FILE__ ) + " this line: " + std::to_string( __LINE__ ) + " message-not-used"
#include <h5cpp/all> 
	...
```
Here is an example of how to capture and handle errors centrally:
```cpp
	// some H5CPP IO routines used in your software
	void my_deeply_embedded_io_calls() {
		arma::mat M = arma::zeros(20,4);
		// compound IO operations in single call: 
		//     file create, dataset create, dataset write, dataset close, file close
		h5::write("report.h5","matrix.ds", M ); 
	}

	int main() {
		// capture errors centrally with the granularity you desire
		try {
			my_deeply_embedded_io_calls();		
		} catch ( const h5::error::io::dataset::create& e ){
			// handle file creation error
		} catch ( const h5::error::io::dataset::write& e ){
		} catch ( const h5::error::io::file::create& e ){
		} catch ( const h5::error::io::file::close& e ){
		} catch ( const h5::any& e ) {
			// print out internally generated error message, controlled by H5CPP_ERROR_MSG macro
			std::cerr << e.what() << std::endl;
		}
	}
```
The detailed CAPI error stack may be unrolled and dumped, muted, unmuted with the provided methods:

* `h5::mute`   - saves current HDF5 CAPI error handler to thread local storage and replaces it with the `NULL` handler, getting rid of all error messages produced by the CAPI. CAVEAT: lean and mean, therefore no nested calls are supported. Should you require more sophisticated handler keep on reading.
* `h5::unmute` - restores previously saved error handler, error messages are handled according to previous handler.

usage:
```cpp
	h5::mute();
	 // ... prototyped part with annoying messages
	 // or the entire application ...
	h5::unmute(); 
```

* `h5::use_errorhandler()` - captures ALL CAPI error messages into thread local storage, replacing current CAPI error handler. This comes in handy when you want to provide details of how the error happened. 

`std::stack<std::string> h5::error_stack()` - walks through underlying CAPI error handler

usage:
```cpp
	int main( ... ) {
		h5::use_error_handler();
		try {
			... rest of the [ single | multi ] threaded application
		} catch( const h5::read_error& e  ){
			std::stack<std::string> msgs = h5::error_stack();
			for( auto msg: msgs )
				std::cerr << msg << std::endl;
		} catch( const h5::write_error& e ){
		} catch( const h5::file_error& e){
		} catch( ... ){
			// some other errors
		}
	}
```

**Design criteria**
- All HDF5 CAPI calls are checked with the only exception of `H5Lexists` where the failure carries information, that the path does not exist yet. 
- Callbacks of CAPI routines doesn't throw any exceptions, honoring the HDF5 CAPI contract, hence allowing the CAPI call to clean up
- Error messages currently are collected in `H5Eall.hpp` and may be customized
- Thrown exceptions are hierarchical
- Only RAII capable/wrapped objects are used, and are guaranteed to clean up through stack unrolling

The exception hierarchy is embedded in namespaces, the chart should be interpreted as a tree, for instance a file create exception is
`h5::error::io::file::create`. Keep [namespace aliasing][3] in mind, which allows you to customize these matters, should you find the long names inconvenient:
```cpp
using file_error = h5::error::io::file
try{
} catch ( const file_error::create& e ){
	// do-your-thing(tm)
}

```
<pre>
h5::error : public std::runtime_error
  ::any               - captures ALL H5CPP runtime errors with the exception of `rollback`
  ::io::              - namespace: IO related error, see aliasing
  ::io::any           - collective capture of IO errors within this namespace/block recursively
      ::file::        - namespace: file related errors
	        ::any     - captures all exceptions within this namespace/block
            ::create  - create failed
			::open    - check if the object, in this case file exists at all, retry if networked resource
			::close   - resource may have been removed since opened
			::read    - may not be fixed, should software crash crash?
			::write   - is it read only? is recource still available since opening? 
			::misc    - errors which are not covered otherwise: start investigating from reported file/line
       ::dataset::    -
			::any
			::create
			::open
			::close
			::read
			::write
			::append
			::misc
      ::packet_table::
			::any
			::create
			::open
			::close
			::read
			::write
			::misc
      ::attribute::
			::any
			::create
			::open
			::close
			::read
			::write
			::misc
    ::property_list::
	  ::rollback
      ::any
	  ::misc
	  ::argument
</pre>

This is a work in progress, if for any reasons you think it could be improved, or some real life scenario is not represented please shoot me an email with the use case, a brief working example.


### Diagnostics  
On occasions it comes in handy to dump the internal state of objects, while currently only `h5::sp_t` data-space descriptor and dimensions supported
in time most of HDF5 CAPI diagnostics/information calls will be added.

```cpp
	h5::ds_t ds =  ... ; 				// obtained by h5::open | h5::create call
	h5::sp_t sp = h5::get_space( ds );  // get the file space descriptor for hyperslab selection
	h5::select_hyperslab(sp,  ... );    // some complicated selection that may fail, and you want to debug
	std::cerr << sp << std::endl;       // prints out the available space
	try { 
		H5Dwrite(ds, ... );            // direct CAPI call fails for with invalid selection
	catch( ... ){
	}
```

### stream operators
Some objects implement `operator<<` to furnish you with diagnostics. In time all objects will the functionality added, for now
only the following objects are supported:

* h5::current_dims_t
* h5::max_dim_t
* h5::chunk_t
* h5::offset_t
* h5::stride_t
* h5::count_t
* h5::block_t
* h5::dims_t
* h5::dt_t
* h5::pt_t
* h5::sp_t

**TODO:** finish article, add implementation for all objects

## Performance
**TODO:** write and run tests



|    experiment                               | time  | trans/sec | Mbyte/sec |
|:--------------------------------------------|------:|----------:|----------:|
|append:  1E6 x 64byte struct                 |  0.06 |   16.46E6 |   1053.87 |
|append: 10E6 x 64byte struct                 |  0.63 |   15.86E6 |   1015.49 |
|append: 50E6 x 64byte struct                 |  8.46 |    5.90E6 |    377.91 |
|append:100E6 x 64byte struct                 | 24.58 |    4.06E6 |    260.91 |
|write:  Matrix<float> [10e6 x  16] no-chunk  |  0.4  |    0.89E6 |   1597.74 |
|write:  Matrix<float> [10e6 x 100] no-chunk  |  7.1  |    1.40E6 |    563.36 |

Lenovo 230 i7 8G ram laptop on Linux Mint 18.1 system

**gprof** directory contains [gperf][1] tools base profiling. `make all` will compile files.
In order to execute install  `google-pprof` and `kcachegrind`.



## Source Code Structure
Follows HDF5 CAPI naming conventions with some minor additions to facilitate linear algebra inclusion as well as compile time reflection.

* **H5F**: File-level access routines.
* **H5G**: Group functions, for creating and operating on groups of objects.
* **H5T**: DataType functions, for creating and operating on simple and compound datatypes to be used as the elements in data arrays.
* **H5S**: DataSpace functions, which create and manipulate the dataspace in which the elements of a data array are stored.
* **H5D**: Dataset functions, which manipulate the data within datasets and determine how the data is to be stored in the file.
* **H5P**: Property list functions, for manipulating object creation and access properties.
* **H5A**: Attribute access and manipulating routines.
* **H5Z**: Compression registration routine.
* **H5E**: Error handling routines.
* **H5I**: Identifier routine.
* **H5M**: Meta descriptors for linear algebra objects

When POD struct are used, the type description  must be sandwiched between the `core` and `io` calls.

* **core**: all routines with the exception of IO calls
* **io** : only IO routines are included, necessary when type definitions are sandwiched
* **all**: all header files are included

Miscellaneous routines are:

* **compat.hpp** to mitigate differences between C++11 and c++17
* **H5capi.hpp** shim between CAPI calls to C++ 
* **H5config.hpp** definitions to control H5CPP behaviour
* **H5cout.hpp** IO stream routines for H5CPP objects, **TODO:** work in progress


## Hacking

### Controlling HDF5 `hid_t` 
H5CPP controls what arguments accepted for various functions calls with `std::enable_if<>::type` mechanism, disabling certain templates being instantiated. The type definition helpers are in `H5std.hpp` file, and here is a brief overview what they do:

* `h5::impl::attr::is_location<class HID_T>::value` checks what handle is considered for attributes as accepted value. By default 
`h5::gr_t | h5::ds_t | h5::dt_t` are set.



[create]: index.md#create
[read]:   index.md#read
[write]:  index.md#write
[append]: index.mdappend
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
