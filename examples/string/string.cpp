/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <vector>
#include <string>

#include <h5cpp/all>
// USE: h5dump example.h5 
// some recent version of hdfview fails with variable length strings on my platform
int main(){
	//RAII will close resource, noo need H5Fclose( any_longer ); 
	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	{
		std::vector<std::string> vec = h5::utils::get_test_data<std::string>(20);
		h5::write(fd, "/strings.txt", vec);
	}
	{
		std::cout << "\nsingle shot entire dataset:\n";
		auto vec = h5::read<std::vector<std::string>>(fd, "strings.txt");
		for( auto i : vec )
			std::cout << i <<"\n";
	}
	{
		std::cout << "\npartial IO start = 3, stride/every=2, count=5 :\n";
		std::cout << "will read in 5 strings, every second starting from third position:\n";
		auto vec = h5::read<std::vector<std::string>>(fd, "strings.txt", h5::offset{2},h5::count{5}, h5::stride{2});
		for( auto i : vec )
			std::cout << i <<"\n";
	}
}
