#include<armadillo>
#include <h5cpp/all>
#include <cstddef>

int main(){
	arma::mat M = arma::zeros(5,6);
	h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC);
	auto ds = h5::write(fd,"some dataset with attributes", M);
	// setting an attribute is simple:
	//ds["prop"] = "this is my first attribute";
	//ds["size"] = M.size();
	//int size = ds["size"];
}

