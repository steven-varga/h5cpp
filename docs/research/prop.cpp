#include <iostream>
#include <hdf5.h>
#include <armadillo>
#include <h5cpp/all>


int main(int argc, char **argv) {

	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC); 
	{
		h5::create<double>(fd,"mat", h5::current_dims{11,7});
	}
	{
		arma::mat mat(11,7);
		hid_t dapl = H5Pcopy( h5::default_dapl );

		hid_t ds = H5Dopen(fd, "mat", dapl);

	}
	return 0;
}

// compile: 
//

