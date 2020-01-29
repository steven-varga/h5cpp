#include <iostream>
#include <hdf5.h>
#include <armadillo>
#include <h5cpp/all>


int main(int argc, char **argv) {

	h5::fd_t fd = h5::create("prop.h5", H5F_ACC_TRUNC); 
	{
		h5::create<double>(fd,"mat", h5::current_dims{11,7}, h5::chunk{3,5} | h5::fill_value<double>{3}  );
	}
	{
		hid_t dapl;
		arma::mat mat(11,7);
		h5::ds_t ds = h5::open(fd,"mat", h5::high_throughput);
		dapl = H5Dget_create_plist( ds );
		std::cout <<"[ is it set? " << H5Pexist(dapl, H5CPP_DAPL_HIGH_THROUGPUT) <<"]\n";
		h5::write(ds, mat, h5::high_throughput, h5::chunk{3,4});
	}
	return 0;
}

// compile: 
//

