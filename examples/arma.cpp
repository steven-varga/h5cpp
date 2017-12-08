#include <armadillo>
#include <h5cpp>

int main(){
	{
		arma::mat M(4,5); 					// create a matrix
		hid_t fd = h5::create("arma.h5"); 	// and a file
			h5::write(fd,"arma/M",M); 		// save dataset
		h5::close(fd); 						// close fd
	}
	{
		hid_t fd = h5::open("arma.h5", H5F_ACC_RDONLY); 	// you can have multiple fd open with H5F_ACC_RDONLY, but single write
		arma::mat M = h5::read<arma::mat>(fd,"arma/M"); 	// read entire edataset back, must have enough free memory
		h5::close(fd); 										// close fd

		M.print();
	}
}


