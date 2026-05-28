#pragma once
#include <string>
#include <vector>
#include <cstdint>
/* typedef is allowed */
typedef unsigned long long int my_uint_t;

namespace sn {
	namespace typecheck {
		struct record_t { /*the types with direct mapping to HDF5*/
			char  _char; unsigned char _uchar; short _short; unsigned short _ushort; int _int; unsigned int _uint;
			long _long; unsigned long _ulong; long long int _llong; unsigned long long _ullong;
			float _float; double _double; long double _ldouble;
			bool _bool;
			// wide characters are not supported in HDF5
			// wchar_t _wchar; char16_t _wchar16; char32_t _wchar32;
		};
	}
	namespace other {
		struct record_t {                    // POD struct with nested namespace
			my_uint_t                   idx; // typedef type 
			my_uint_t                    aa; // typedef type 
			double              field_02[3]; // const array mapped 
			typecheck::record_t field_03[4]; //
		};
	}
	namespace example {
		struct record_t {                    // POD struct with nested namespace
			my_uint_t                     idx; // typedef type 
			float              field_02[7]; // const array mapped 
			sn::other::record_t field_03[5]; // embedded Record
			sn::other::record_t field_04[5]; // must be optimized out, same as previous
			other::record_t  field_05[3][8]; // array of arrays 
		};
	}
	namespace not_supported_yet {
		// NON POD: not supported in phase 1
		// C++ Class -> PODstruct -> persistence[ HDF5 | ??? ] -> PODstruct -> C++ Class 
		struct container_t {
			double                              idx; // 
			std::string                    field_05; // c++ object makes it non-POD
			std::vector<example::record_t> field_02; // ditto
		};
	}
	/* BEGIN IGNORED STRUCT */
	// these structs are not referenced with h5::read|h5::write|h5::create operators
	// hence compiler should ignore them.
	struct ignored_record_t {
		signed long int   idx;
		float        field_0n;
	};
	/* END IGNORED STRUCTS */
}
