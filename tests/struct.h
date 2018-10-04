#include <hdf5.h>
#include <h5cpp/H5misc.hpp>

namespace SomeNameSpace {
	typedef struct StructType {
		unsigned int   field1;
		double 			field2;
		float 			field3;
		char 			field4;
		double 			field5;
		double 			field6;
		double 			field7;
		double 			field8;
		double 			field9;
	} struct_type;
}
namespace sn = SomeNameSpace;
/** make sure to register the structure
 */
H5CPP_REGISTER_STRUCT( sn::struct_type );

namespace h5{ namespace utils {
	// specialize provided template and define HDF5 layout
	template<> hid_t h5type<sn::struct_type>(){
			hid_t type = H5Tcreate(H5T_COMPOUND, sizeof (sn::struct_type));
			// layout
			H5Tinsert(type, "field1", 	HOFFSET(sn::struct_type, field1), H5T_NATIVE_UINT16);
			H5Tinsert(type, "field2", 	HOFFSET(sn::struct_type, field2), H5T_NATIVE_DOUBLE);
			H5Tinsert(type, "field3", 	HOFFSET(sn::struct_type, field3), H5T_NATIVE_FLOAT);
			H5Tinsert(type, "field4", 	HOFFSET(sn::struct_type, field4), H5T_NATIVE_CHAR);
			H5Tinsert(type, "field5", 	HOFFSET(sn::struct_type, field5), H5T_NATIVE_DOUBLE);
			H5Tinsert(type, "field6", 	HOFFSET(sn::struct_type, field6), H5T_NATIVE_DOUBLE);
			H5Tinsert(type, "field7", 	HOFFSET(sn::struct_type, field7), H5T_NATIVE_DOUBLE);
			H5Tinsert(type, "field8", 	HOFFSET(sn::struct_type, field8), H5T_NATIVE_DOUBLE);
			H5Tinsert(type, "field9", 	HOFFSET(sn::struct_type, field9), H5T_NATIVE_DOUBLE);
            return type;
	};
}}

namespace h5 { namespace utils { // this specializationis not necessary, only used in tests 
	template <> std::vector<sn::StructType> get_test_data( size_t n ){
		std::vector<sn::StructType> data;
		for(unsigned int i; i<n; i++)
			data.push_back(	sn::StructType( {i,0.0,0,0} ));
		return data;
	}
}}

