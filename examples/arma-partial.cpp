#include <armadillo>
#include <h5cpp/all>

int main(){
	// create hdf5 container/file
	hid_t fd = h5::create("arma.h5");

	arma::vec data = {1,2,3,4,5};
	hid_t ds = h5::create<float>(fd,"/arma/data",{5,5,1,2});

	// offset - where to start within the array, in this case is {0,0,0,0}
	// count  - how many elements we would like to write: {a,b,c} 
	h5::write(ds, data, {0,0,0,0},{1,5,1,1} ); 			//
	// we only want 3 elements back from offset 1, the rank must be 4
	auto rvo  = h5::read<arma::mat>(ds,{0,1,0,0},{1,3,1,1});
	rvo.print();

	H5Dclose(ds);
	h5::close(fd);
}


