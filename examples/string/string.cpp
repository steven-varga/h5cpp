/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#include <vector>
#include <string>

#include <h5cpp/all>

int main(){
	//RAII will close resource, noo need H5Fclose( any_longer ); 
	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);
	{
		std::vector<std::string> vec = h5::utils::get_test_data<std::string>(20);
		h5::write(fd, "/strings.txt", vec);
	}
	{
		auto vec = h5::read<std::vector<std::string>>(fd, "strings.txt");
		for( auto i : vec ) 
			std::cout << i <<"\n";
	}
}
