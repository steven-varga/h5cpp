#include <iostream>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <h5cpp/all>

#include <boost/numeric/ublas/io.hpp>

using namespace std;
// 
template<class T> using Matrix = boost::numeric::ublas::matrix<T>;
template<class T> using Colvec = boost::numeric::ublas::vector<T>;


int main(){
	{ // CREATE - WRITE
		Matrix<double> M(4,5); 				// create a matrix
		hid_t fd = h5::create("ublas.h5"); 	// and a file
			h5::write(fd,"ublas/M",M); 		// save dataset
		h5::close(fd); 						// close fd
	}
	{ // CREATE - READ 
		hid_t fd = h5::open("ublas.h5", H5F_ACC_RDWR); 				// you can have multiple fd open with H5F_ACC_RDONLY, but single write
			hid_t ds = h5::create<double>(fd,"dataset",{3,2});		// create dataset, default to NaN-s
			auto M = h5::read<Matrix<double>>( ds ); 				// read data back as matrix
			std::cout << M <<std::endl;
		h5::close(fd); 												// close fd
	}
	{ // READ
		Matrix<double> M = h5::read<Matrix<double>>("ublas.h5","ublas/M"); 	// read entire dataset back with a single read
		std::cout << M<<std::endl;
	}
	{
		// PARTIAL READ
		hid_t fd = h5::create("ublas-partial.h5");
		Colvec<float> data(5);  for( int i=0; i<5; i++ ) data[i] = i;
		hid_t ds = h5::create<float>(fd,"/ublas/data",{5,5,1,2});

		// offset - where to start within the array, in this case is {0,0,0,0}
		// count  - how many elements we would like to write: {a,b,c} 
		h5::write(ds, data, {0,0,0,0},{1,5,1,1} ); 			//
		// we only want 3 elements back from offset 1, the rank must be 4
		auto V  = h5::read<Colvec<double>>(ds,{0,1,0,0},{1,3,1,1});
		std::cout << V << std::endl;

		H5Dclose(ds);
		h5::close(fd);
	}
}



