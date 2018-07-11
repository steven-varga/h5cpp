#include <armadillo>
#include <h5cpp/all>


// see: https://support.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetTypeConvCb
H5T_conv_ret_t handler( H5T_conv_except_t except_type, hid_t src_id, hid_t dst_id, void *src_buf, void *dst_buf, void *op_data){
	return H5T_CONV_HANDLED;
}

int main(){
	{ // CREATE - WRITE
		h5::fd_t fd = h5::create("arma.h5",H5F_ACC_TRUNC);
		arma::mat M(4,7); M.ones();				// create a matrix

		h5::write(fd,"transform",  M,
		// data transform provides linear transformation from character descriptions
		// linear operators {*,/,+,-} allowed on x variable: 
				h5::data_transform{"2*x+5"} );
	}
	{ // READ back, pass conversion handler -- if internal conversion fails, and a 
	  // data transform expression 	
		auto m = h5::read<arma::fmat>("arma.h5","transform", 
			 h5::type_conv_cb{ handler, nullptr } | h5::data_transform{"x/2 + 1"});
		m.print();
	}

}


