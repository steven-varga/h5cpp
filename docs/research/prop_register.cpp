#include <iostream>
#include <hdf5.h>
#include <armadillo>
#include <h5cpp/all>

#define CUSTOM_PROP "custom_prop"

namespace h5 { namespace exp {

	struct custom_prop_t {
		custom_prop_t(){ std::cout <<"<CTOR>"; }
		~custom_prop_t(){ std::cout <<"<DTOR>"; }
	};


	::herr_t dapl_class_create_func( ::hid_t prop_id, void* ptr){
		std::cout <<"<create func>\n";
		return 1;
	}
	::herr_t dapl_class_copy_func( ::hid_t id, void* ptr){
		std::cout <<"<prop copy>\n";
		return 1;
	}
	::herr_t dapl_class_close_func( ::hid_t id, void* ptr){
		std::cout << "<prop comp>\n";
		return 1;
	}

	// hid_t H5Pcreate_class( hid_t parent_class, const char *name, H5P_cls_create_func_t create, void *create_data, H5P_cls_copy_func_t copy, void *copy_data, H5P_cls_close_func_t close, void *close_data )

	::herr_t dapl_create_class(  ){
		return H5Pcreate_class( H5P_DATASET_ACCESS, "H5CPP_DATASET_ACCESS",
				dapl_class_create_func,nullptr,
				dapl_class_copy_func,nullptr,
				dapl_class_close_func,nullptr);
	}


}}

const static ::hid_t H5CPP_DATASET_ACCESS_CLASS = h5::exp::dapl_create_class();

int main(int argc, char **argv) {

	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC); 
	{
		arma::mat mat(7,11);
		h5::create<double>(fd,"mat", h5::current_dims{11,7});
	}
	{
		h5::dapl_t dapl = H5Pcreate(H5CPP_DATASET_ACCESS);
		h5::ds_t ds = h5::open(fd,"mat", dapl);
	}
	return 0;
}

// compile: 
//

