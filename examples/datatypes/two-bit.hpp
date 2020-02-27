/* Copyright (c) 2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

// do your thing, usually this would be in a separate header file
namespace bitstring {
	struct two_bit { // wrapper to aid C++ template mechanism, zero runtime cost
		// but you have to develop your own efficient two bit arithmetic
		// as well as in memory storage mechanism, this is a 4 x 2bit data block
		unsigned operator[]( int idx_ ) const {
			switch (idx_){
				case 0: return (value & 0b0000'0011) >> 0;
				case 1: return (value & 0b0000'1100) >> 2;
				case 2: return (value & 0b0011'0000) >> 4;
				case 3: return (value & 0b1100'0000) >> 6;
				default:
						throw std::runtime_error("out of bound");
			}
		}
		two_bit( unsigned char value_ ) : value(value_){
		}
		two_bit() = default;
		unsigned char value;
	};
}

// BEGIN H5CPP SPECIFIC CUSTOM TYPE DEFINITION
namespace h5::impl::detail {
	template <> struct hid_t<bitstring::two_bit, H5Tclose,true,true, hdf5::type> : public dt_p<bitstring::two_bit> {
		using parent = dt_p<bitstring::two_bit>;  // h5cpp needs the following typedefs
		using parent::hid_t;
		using hidtype = bitstring::two_bit;

		// opaque doesn't care of byte order, also since you are using single byte
		// it is not relevant
		hid_t() : parent( H5Tcreate( H5T_OPAQUE, 1) ) { // 1 == single byte, i would pack it into 64 bit though
			H5Tset_tag(handle, "bitstring::two_bit");
			hid_t id = static_cast<hid_t>( *this );
		}
	};
}
namespace h5 {
	template <> struct name<bitstring::two_bit> {
		static constexpr char const * value = "bitstring::two_bit";
	};
}
// END H5CPP SPECIFIC TYPE DEFINEITION


// SERVICE ROUTINES
std::ostream& operator<<(std::ostream& os, const bitstring::two_bit& val){
	os << val[3] << " " << val[2] << " " << val[1] <<" " << val[0];
	return os;
}
