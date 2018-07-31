Greetings!

Did you try [H5CPP](http://h5cp.ca), an easy to use compiler assisted template library that introduces relaxed function prototypes, optimized/profiled HDF5 CAPI calls and hides the details so you can start right away? 
While the  library is under heavy development it is functional, supports major linear algebra systems as well as arbitrary deep POD types with seamless compiler assistance, and the STL vector. 

How simple can it get?
```cpp
   h5::write( "arma.h5", "arma vec inside matrix",  V // object contains 'count' and rank being written
            ,h5::current_dims{40,50}  // control file_space directly where you want to place vector
            ,h5::offset{5,0}            // when no explicit current dimension given current dimension := offset .+ object_dim .* stride (hadamard product)  
            ,h5::stride{4,4}, h5::block{3,3}
            ,h5::max_dims{40,H5S_UNLIMITED} )
// wouldn't it be nice to have unlimited dimension? 
// if no explicit chunk is set, then the object dimension is
// used as unit chunk
        
```
The above example is to demonstrate partial create + write with extendable datasets which can be turned into high performance packet table: `h5::pt_t ` by a simple conversion `h5::pt_t pt = h5::open( ... )` ,  `h5::pt_t pt = h5::create(...)` or `h5::pt_t pt = ds` where `h5::ds_t ds = ... `.   


`h5::create | h5::read | h5::write | h5::append` take RAII enabled descriptors or CAPI hid_t ones -- depending on conversion policy: seamless or controlled fashion. The optional arguments may be placed in any order, compile time computed and come with intelligent compile time error messages.

```cpp
h5::write( "arma.h5", "arma vec inside matrix",  V 
      ,h5::stride{4,4}, h5::block{3,3}, h5::current_dims{40,50} 
      ,h5::offset{5,0}, h5::max_dims{40,H5S_UNLIMITED}  );
```
Wouldn't it be nice not to have to  remember everything? Focus on the idea, write it out intuitively and refine it later. The function construct below compiles into the same instructions as above. 

```cpp
h5::write( "arma.h5", "arma vec inside matrix",  V 
      ,h5::max_dims{40,H5S_UNLIMITED}, h5::stride{4,4},  h5::current_dims{40,50}
      ,h5::block{3,3}, h5::offset{5,0},   );
```

Why should you know about HDF5 at all, isn't it work about ideas? the details can be filled in later when needed:
```cpp 
// supported objects:  raw pointers | armadillo | eigen3 | blaze | blitz++ | it++ | dlib | uBlas | std::vector
arma::vec V(4); // some vector: armadillo, eigen3, 
h5::write( "arma.h5", "path/to/dataset name",  V);
```
How about some really complicated POD struct type that your client or colleague  wants to see in action right now?
Invoke clang++ based `h5cpp` compiler on the translation unit -- group of files which are turned into a single object file -- and call it done!
```cpp
namespace sn {
	namespace typecheck {
		struct Record { /*the types with direct mapping to HDF5*/
			char  _char; unsigned char _uchar; short _short; unsigned short _ushort; int _int; unsigned int _uint;
			long _long; unsigned long _ulong; long long int _llong; unsigned long long _ullong;
			float _float; double _double; long double _ldouble;
			bool _bool;
			// wide characters are not supported in HDF5
			// wchar_t _wchar; char16_t _wchar16; char32_t _wchar32;
		};
	}
	namespace other {
		struct Record {                    // POD struct with nested namespace
			MyUInt                    idx; // typedef type 
			MyUInt                     aa; // typedef type 
			double            field_02[3]; // const array mapped 
			typecheck::Record field_03[4]; //
		};
	}
	namespace example {
		struct Record {                    // POD struct with nested namespace
			MyUInt                    idx; // typedef type 
			float             field_02[7]; // const array mapped 
			sn::other::Record field_03[5]; // embedded Record
			sn::other::Record field_04[5]; // must be optimized out, same as previous
			other::Record  field_05[3][8]; // array of arrays 
		};
	}
	namespace not_supported_yet {
		// NON POD: not supported in phase 1
		// C++ Class -> PODstruct -> persistence[ HDF5 | ??? ] -> PODstruct -> C++ Class 
		struct Container {
			double                            idx; // 
			std::string                  field_05; // c++ object makes it non-POD
			std::vector<example::Record> field_02; // ditto
		};
	}
	/* BEGIN IGNORED STRUCT */
	// these structs are not referenced with h5::read|h5::write|h5::create operators
	// hence compiler should ignore them.
	struct IgnoredRecord {
		signed long int   idx;
		float        field_0n;
	};
	/* END IGNORED STRUCTS */
```
I did try to make the above include file  as  ugly and complicated as I could. But do you really need to read it? What if you had a technology at your disposal that can do it for you?  
In your code all you have to do is to trigger the compiler by making any `h5::` operations on the desired data structure. It works without the hmmm boring details?
```cpp
...
std::vector<sn::example::Record> vec 
    = h5::utils::get_test_data<sn::example::Record>(20);
// mark vec  with an h5:: operator and delegate 
// the details to h5cpp compiler
h5::write(fd, "orm/partial/vector one_shot", vec );
...
``` 
H5CPP is a novel approach to  persistence in the field of machine learning, it provides high performance sequential and block access to HDF5 containers through modern C++. If you liked what you see please come and see our presentation in [Chicago C++ usergroup meeting on July 31](https://www.meetup.com/Chicago-C-CPP-Users-Group/) 

best wishes:
steven varga
the author of h5cpp


