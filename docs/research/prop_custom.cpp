#include <iostream>
#include <hdf5.h>
#include <armadillo>
#include <h5cpp/all>

#define H5CPP_DATASET_ACCESS_PROP "h5cpp_dapl_prop"
#define H5CPP_DATASET_ACCESS_CLASS "h5cpp_dataset_access"

namespace h5 { namespace exp {

	struct custom_prop_t {
		custom_prop_t(){ std::cout <<"<CTOR>"; }
		~custom_prop_t(){ std::cout <<"<DTOR>"; }
	};

	/* --------------  custom class  --------------- */
	::herr_t dapl_class_create_func( ::hid_t prop_id, void* ptr){
		std::cout <<"<class create>\n";
		return 1;
	}
	::herr_t dapl_class_copy_func( ::hid_t id_to, ::hid_t id_from, void* ptr){
		std::cout <<"<class copy>\n";
		return 1;
	}
	::herr_t dapl_class_close_func( ::hid_t id, void* ptr){
		std::cout << "<class close>\n";
		return 1;
	}

	/* --------------  custom prop register  --------------- */
	::herr_t dapl_create_prop( const char *name, size_t size, void *initial_value){
		std::cout << "<prop create>\n";
		return 0;
	}
	::herr_t dapl_set_prop( hid_t prop_id, const char *name, size_t size, void *new_value){
		std::cout << "<prop set>\n";
		return 0;
	}
	::herr_t dapl_get_prop( hid_t prop_id, const char *name, size_t size, void *value){
		std::cout << "<prop get>\n";
		return 0;
	}

	::herr_t dapl_delete_prop( hid_t prop_id, const char *name, size_t size, void *value){
		std::cout << "<prop delete>\n";
		return 0;
	}
	::herr_t dapl_copy_prop( const char *name, size_t size, void *value){
		std::cout << "<prop copy>\n";
		return 0;
	}
	int dapl_compare_prop( const void *value1, const void *value2, size_t size){
		std::cout << "<prop compare>\n";
		return 0;
	}
	::herr_t dapl_close_prop( const char *name, size_t size, void *value){
		std::cout << "<prop close>\n";
		return 0;
	}

	/* --------------  custom prop utils  --------------- */
	::herr_t dapl_register_prop(::hid_t clazz){
		custom_prop_t prop;
		return H5Pregister2( clazz, H5CPP_DATASET_ACCESS_PROP, sizeof(custom_prop_t), &prop,
				dapl_create_prop, dapl_set_prop, dapl_get_prop, dapl_delete_prop, dapl_copy_prop, dapl_compare_prop, dapl_close_prop );
	}

	::hid_t dapl_create_class(  ){
		hid_t clazz = H5Pcreate_class( H5P_DATASET_ACCESS, H5CPP_DATASET_ACCESS_CLASS,
				dapl_class_create_func,nullptr, dapl_class_copy_func,nullptr, dapl_class_close_func,nullptr);
		dapl_register_prop( clazz );
		return clazz;
	}
}}

namespace h5 { namespace exp {
	const static ::hid_t H5CPP_DATASET_ACCESS = h5::exp::dapl_create_class();
	const static ::hid_t default_dapl = H5Pcreate( H5CPP_DATASET_ACCESS );
}}




int main(int argc, char **argv) {

	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC); 
	{
		h5::create<double>(fd,"mat", h5::current_dims{11,7});
	}
	{
		arma::mat mat(11,7);
		hid_t dapl = H5Pcopy( h5::exp::default_dapl );
		h5::exp::custom_prop_t prop;
		H5Pset(dapl,H5CPP_DATASET_ACCESS_PROP, &prop );
		std::cout << "check prop if set on dapl passed to h5::open: "
			<< H5Pexist(dapl, H5CPP_DATASET_ACCESS_PROP ) << "\n\n";

		hid_t ds = H5Dopen(fd, "mat", dapl);
		hid_t dapl_ds = H5Dget_access_plist( ds );

		std::cout << "custom property expected be present in retrieved dapl: "
			<< H5Pexist(dapl_ds, H5CPP_DATASET_ACCESS_PROP ) << "\n";
		std::cout <<"\n\n";

		// lets set this property on the retrieved dapl_ds
		H5Pset(dapl_ds,H5CPP_DATASET_ACCESS_PROP, &prop );
		hid_t dapl_confirm = H5Dget_access_plist( ds );

		std::cout << "did change persist? : "
			<< H5Pexist(dapl_confirm, H5CPP_DATASET_ACCESS_PROP ) << "\n";
		std::cout <<"\n\n";
	}
	return 0;
}

// compile: 
//

