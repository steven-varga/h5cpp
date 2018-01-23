#include <vector>
#include <armadillo>
#include <h5cpp/all>
#include <h5cpp/mock.hpp>

using namespace h5::utils;

int main(int argc, char **argv) {
	arma::wall_clock timer;

	hid_t fd = h5::create("string.h5");

	hid_t ds = h5::create<std::string>(fd,"/string vector",{H5S_UNLIMITED},{1024}, 0);
	timer.tic();

	long long size = 100'000ll; 
	std::vector<std::string> data = get_test_data<std::string>(size); 	//obtain a set of random length strings 

	{ // WRITE string data one shot into hdf5 dataset 
		h5::write(fd,"/name.str", data); 
	}
	{ // PARTIAL WRITE string data into hdf5 dataset 
		hid_t ds = h5::create<std::string>(fd, "/string.mat",{6,6},{2,2}, 9); 	// gzipped chunked {6x6} matrix<std::string> data-set 
			h5::write(ds, data, {2,1},{3,2} ); 									// partial write a {3x2} block to coordinates: {2x1}
		H5Dclose(ds); 
	}
	{ // partial read of string
		std::vector<std::string> data = 
			h5::read<std::vector<std::string>>(fd,"/string.mat", {2,1},{3,2} ); // partial write a {3x2} block to coordinates: {2x1}
		int j=0;
		std::cout<<"PARTIAL READ: \n\t";
		for(auto i:data ) std::cout <<"[" <<j++<<"] " << i << " ";
		std::cout<<std::endl;
	}
	H5Dclose(ds);
	h5::close(fd);
	{ // write STL vector
		hid_t fd = h5::open("string.h5", H5F_ACC_RDWR);
		h5::write(fd,"/all",data);
		h5::close(fd);
	} 
	{ // READ it back later

		using h5string = std::vector<std::string>;
		hid_t fd = h5::open("./string.h5",H5F_ACC_RDWR);
		try {
			h5string all = h5::read<h5string>(fd, "/all");
			std::cout<<"FULL READ:  \n\t";
			for(int i=0;i<6;i++) std::cout << "[" <<i <<"] " << all[i] <<" ";
			std::cout<<std::endl;
		} catch( std::runtime_error ex ){
			std::cerr <<"error: " << ex.what();
		}
		h5::close( fd );
	}
	{ // READ back non existing dataset
		using h5string = std::vector<std::string>;
		hid_t fd = h5::open("./string.h5",H5F_ACC_RDWR);
		h5::mute(); // turn off diagnostic messages
		try {

			std::cerr <<"Trying to open a nonexisting dataset, which should result in an exception thrown: \n";
			h5string all = h5::read<h5string>(fd, "/path to non exixsting dataset");
		} catch( std::runtime_error ex ){
			std::cerr <<"RUNTIME ERROR: " << ex.what() <<std::endl<<std::endl;
			std::cerr <<"[the previous runtime exception happened on purpose]\n";
		}
		h5::close( fd );
	}
}


