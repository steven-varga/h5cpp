# H5CPP Type System
<img src="../icons/cpp_type_system.png" alt="some text" style="zoom:60%;" />


# Type Traits for internal use  
withing `h5::impl` namspace the following traits are defined:

- `is_container<T>::value` returns `true` if `T` is an STL container type
- `is_adaptor<T>::value` returns `true` if `T` is an STL adaptor type
- `is_stl<T>::value` returns `true` when  `T` is a container or an adaptor type
- `is_rank<T,N>::value` returns `true` if rank of `T` is equal with `N`
- `is_scalar<T>::value`, `is_vector<T>::value`, `is_matrix<T>::value`, `is_cube<T>::value` are shortcuts to `is_rank`
- `decay<T>::type` returns the greatest homogeneous element type of `T` objects
- `has_attribute<T>::value` where `T` is HDF5 | H5CPP handle, `true` when takes an attribute
- `is_location<T>::value`  where `T` is HDF5 | H5CPP handle, `true` if it is a valid path
- `is_linag<T>::value` containers which are limited to numerical types

template<class T, class... Ts> data; 
template<class T, class... Ts> std::array<size_t,N> size( const T& ref ); 
template <class T, class... Ts> struct get {
		static inline T ctor( std::array<size_t,0> dims ){
			return T(); }};



| sequence containers   | c++ layout | shape       |
|-----------------------|------------|-------------|
|`std::array<T>`        | standard   | vector      |
|`std::vector<T>`       | standard   | hypercube   |
|`std::deque<T>`        | no         | ragged      |
|`std::forward_list`    | no         | ragged      |
|`std::list<T>`         | no         | ragged      |


| associative containers   | c++ layout | shape       |
|-----------------------|------------|-------------|
	| `std::set<T>`         | | 

| HDF5 data type  | STL like containers | dataset shape|
|-----------------|---------------------|--------------|
|bitfield         | `std::vector<bool>`   | ragged     |
|VL string        | `sequential<std::string>` | ragged |

|arithmetic       | std::vector<T>      | regular      |

|STL like objects | HDF5 data type | dataset shape|
|-----------------|----------------|--------------|

# Objects and type classification
```
arithmetic ::= (signed | unsigned)[char | short | int | long | long int, long long int] 
					  | [float | double | long double]
reference ::= [ pointer | R value reference | PR value reference]
```

## What you need to know of C++ data types
The way objects arranged in memory is called the layout. The C++ memory model is more relaxed than the one in C or Fortran therefore one can't assume contiguous arrangement of class members, or even being of the same order as defined. Since data transfer operation in HDF5 require contiguous memory arrangement which creates a mismatch between the two systems.  C++ objects may be categorized by memory layout such as:  

* **Trivial:** class members are in contiguous memory, but order is not guaranteed; consistent among same compiles
	* no virtual functions or virtual base classes,
	* no base classes with a corresponding non-trivial constructor/operator/destructor
	* no data members of class type with a corresponding non-trivial constructor/operator/destructor

* **Standard:** contiguous memory, class member order is guaranteed, good for interop
	* no virtual functions or virtual base classes
	* all non-static data members have the same access control
	* all non-static members of class type are standard-layout
	* any base classes are standard-layout
	* has no base classes of the same type as the first non-static data member.

	meets one of these conditions:
	
	* no non-static data member in the most-derived class and no more than one base class with non-static data members, or
	has no base classes with non-static data members

<div id="object" style="float: right">
	<img src="../icons/struct.svg" alt="some text" style="zoom:60%;" />
</div>
* **POD or plain old data:** members are contiguous memory location and member variables are in order although may be padded to alignment.
Standard layout C++ classes and POD types are suitable for direct interop with the CAPI. Depending the content these types may be categorized as homogeneous  and struct types.

Some C++ classes are treated special, as they *almost* fulfill the **standard layout** requirements. Linear algebra libraries with data structures supporting
 BLAS/LAPACK calls ie: `arma::Mat<T>`, or STL like objects with contiguous memory layout  such as `std::vector<T>`, `std::array<T>` may be 
 converted into Standard Layout Type by providing a shim code to grab a pointer to the underlying memory and size. Indeed the previous versions of H5CPP
 had been supporting only objects where the underlying data could be easily obtained.


# HDF5 dataset shapes

## Scalars, Vectors, Matrices and Hypercubes
<div id="object" style="float: left">
	<img src="../icons/colvector.svg" alt="some text" style="zoom:40%;" />
	<img src="../icons/matrix.svg" alt="some text" style="zoom:50%;" />
	<img src="../icons/hypercube.svg" alt="some text" style="zoom:40%;" />
</div>
Are the most frequently used objects, and the cells may take up any fixed size data format. STL like Sequential and Set containers as well as C++ built in arrays may be mapped 0 - 7 dimensions of HDF5   homogeneous, and regular in shape data structure. Note that `std::array<T,N>` requires the size `N` known at compile time, therefore it is only suitable for partial IO read operations.
```
T::= arithmetic | pod_struct
```
- `std::vector<T>`
- `std::array<T>`
- `T[N][M]...`


</br>

## Ragged Arrays
*VL datatypes are designed **allow** the amount of **data** stored **in each element of a dataset to vary**. This change could be over time as new values, with different lengths, were written to the element. Or, the change can be over "space" - the dataset's space, with each element in the dataset having the same fundamental type, but different lengths. **"Ragged arrays" are the classic example of elements that change over the "space" of the dataset.** If the elements of a dataset are not going to change over "space" or time, a VL datatype should probably not be used.*

- **Access Time Penalty**
Accessing VL information requires reading the element in the file, then using that element's location information to retrieve the VL information itself. In the worst case, this obviously doubles the number of disk accesses required to access the VL information.

	When VL information in the file is re-written, the old VL information must be released, space for the new VL information allocated and the new VL information must be written to the file. This may cause additional I/O accesses.


- **Storage Space Penalty**
Because VL information must be located and retrieved from another location in the file, extra information must be stored in the file to locate each item of VL information (i.e. each element in a dataset or each VL field in a compound datatype, etc.). Currently, that extra information amounts to 32 bytes per VL item.

- **Chunking and Filters**
Storing data as VL information has some effects on chunked storage and the filters that can be applied to chunked data. Because the data that is stored in each chunk is the location to access the VL information, the actual VL information is not broken up into chunks in the same way as other data stored in chunks. Additionally, because the actual VL information is not stored in the chunk, any filters which operate on a chunk will operate on the information to locate the VL information, not the VL information itself.

- **MPI-IO**
Because the parallel I/O file drivers (MPI-I/O and MPI-posix) don't allow objects with varying sizes to be created in the file, attemping to create a dataset or attribute with a VL datatype in a file managed by those drivers will cause the creation call to fail.

```
T::= arithmetic | pod_struct | pod_struct 
element_t ::= std::string | std::vector<T> | std::list<T> | std::forward_list 
```
<div id="object" style="float: right">
	<img src="../icons/ragged.svg" alt="some text" style="zoom:80%;" />
</div>
Sequences of variable lengths are mapped to HDF5 ragged arrays, a data structure with the fastest growing dimension of variable length. The C++ equivalent is a container within a sequential container -- with embedding limited to one level. 

- `std::vector<element_t>`
- `std::array<element_t,N>`
- `std::list<element_t>`
- `std::forward_list<element_t>`
- `std::stack, std::queue, std::priority_queue`


# Mapping C++ Non Standard Layout Classes
Since the class member variables are non-consecutive memory locations the data transfer needs to be broken into multiple pieces.

- `std::map<K,V>`
- `std::multimap<K,V>`
- `std::unordered_map<K,V>`
- `std::unordered_multimap<K,V>`
- `arma::SpMat<T>` sparse matrices in general
- all non-standard-layout types



## multiple homogeneous datasets
<img src="../icons/key-value.svg" alt="some text" style="zoom:100%;" />


## single dataset: compound data type
<img src="../icons/stream_of_struct.svg" alt="some text" style="zoom:100%;" />

multiple records



