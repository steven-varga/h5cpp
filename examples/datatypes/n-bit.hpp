/* Copyright (c) 2020 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

// do your thing, usually this would be in a separate header file
namespace bitstring {
	struct n_bit { // wrapper to aid C++ template mechanism, zero runtime cost
		// allow conversion: auto value =  static_cast<unsigned char>( n_bit_type );
		explicit operator unsigned char() const {
			return value;
		}
		explicit operator unsigned int() const {
			return value;
		}
		n_bit() = default;
		n_bit( unsigned char value_ ) : value(value_){}
		unsigned char value;
	};
}

// BEGIN H5CPP SPECIFIC CUSTOM TYPE DEFINITION
namespace h5::impl::detail {
	template <> struct hid_t<bitstring::n_bit, H5Tclose,true,true, hdf5::type> : public dt_p<bitstring::n_bit> {
		using parent = dt_p<bitstring::n_bit>;  // h5cpp needs the following typedefs
		using parent::hid_t;
		using hidtype = bitstring::n_bit;

		// opaque doesn't care of byte order, also since you are using single byte
		// it is not relevant
		hid_t() : parent( H5Tcopy( H5T_NATIVE_UCHAR) ) {
			H5Tset_precision(handle, 2);
			hid_t id = static_cast<hid_t>( *this );
		}
	};
}
namespace h5 {
	template <> struct name<bitstring::n_bit> {
		static constexpr char const * value = "bitstring::n_bit";
	};
}
// END H5CPP SPECIFIC TYPE DEFINEITION


// SERVICE ROUTINES
std::ostream& operator<<(std::ostream& os, const bitstring::n_bit& data){
	os << data.value;
	return os;
}

std::ostream& operator<<(std::ostream& os, const
	 Eigen::Matrix<bitstring::n_bit, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>& M){
	for( size_t i=0; i<M.rows(); i++){
	   	for( size_t j=0; j<M.cols(); j++) os << static_cast<unsigned int>( M(i,j)) <<" ";
		os<<std::endl;
	}
	return os;
}

