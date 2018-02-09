#include <vector>
#include <h5cpp/all>
#include <iostream>

#define values 0,1,2,3,4,5,6,7,8,9
using namespace std;

int main(){

	std::vector<char> vc = {values};
	std::vector<unsigned char> vuc = {values};
	std::vector<short> vs = {values};
	std::vector<unsigned short> vus = {values};
	std::vector<int> vi = {values};
	std::vector<unsigned int> vui = {values};
	std::vector<float> vf = {values};
	std::vector<double> vd = {values};
	std::vector<long double> vld = {values};
	std::vector<std::string> vstr = {"first","second","third","forth","..."};

	auto fd = h5::create("stl.h5");
		// write entire vector into non-existing dataset

		h5::write(fd,"/stl/char",vc);
		h5::write(fd,"/stl/uchar",vuc);
		h5::write(fd,"/stl/short",vs);
		h5::write(fd,"/stl/ushort",vus);
		h5::write(fd,"/stl/int",vi);
		h5::write(fd,"/stl/uint",vui);
		h5::write(fd,"/stl/float",vf);
		h5::write(fd,"/stl/double",vd);
		h5::write(fd,"/stl/long double",vld);
		h5::write(fd,"/stl/string",vstr);

		{//  re-write datasets
		std::fill(std::begin(vc),std::end(vc),-1);
		h5::write(fd,"/stl/char",vc);
		vstr[2] = "overwrite...";
		h5::write(fd,"/stl/string",vstr);
		}
		{//partial write of a compressed matrix, keep in mind C is row major, and HDF5 fastest growing dimension is the last
			hid_t ds = h5::create<float>(fd,"stl/partial",{100,vf.size()},{1,10}, 9);
				h5::write(ds, vf, {2,0},{1,10} ); // write data into 4th row
				h5::write(ds, vf, {2,2},{2,5} );  // write data into 4th row
				h5::write(ds, vf, {2,2},{10,1} ); // write data into 4th row
			// RVO or return value optimization will not make a copy
			auto&& all_data_rvo  = h5::read< std::vector<float>>(ds);
			auto all_data_rvo_also_ok  = h5::read< std::vector<float>>(ds);
			// check if reference is read-write
			all_data_rvo[2] = 333;
			//for( auto i: all_data_rvo ) std::cout<< i <<" "; std::cout<<"\n";
			H5Dclose(ds);
			//partial read while converting 'float' to 'double':
			auto pr = h5::read<std::vector<double>>(fd,"stl/partial", {2,0},{1,10});
		}
		{
		hid_t ds = h5::create<std::string>(fd,"stl/string matrix",{10,10},{1,10},9);
			std::vector<std::string> data = {"A","B","C","D", "E","F","G","H"};
			h5::write(ds, data, {0,0},{1,data.size()} ); 	// write data into {0,0} with shape {1,size} 
			h5::write(ds, data, {2,2},{2,4} ); 				// write data into {2,2} with shape {2,4}

			// read entire dataset back into a single vector:
			std::vector<std::string> all_str = h5::read<std::vector<std::string>>(ds);
			for( auto i:all_str) std::cout<<i<<","; std::cout<<std::endl;

			//std::vector<std::string> all_str_prt = h5::read<std::vector<std::string>>(ds,{1,0}, {1,10});
			//for( auto i:all_str_prt) std::cout<<i<<","; std::cout<<std::endl;

			H5Dclose(ds);
		h5::close(fd);
		}

}
#undef values

