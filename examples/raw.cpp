#include <h5cpp>
#include <cstddef>

#define values 0,1,2,3,4,5,6,7,8,9

int main(){
	hid_t fd = h5::create("base.h5");
	float  af[] = {values};
	double ad[] = {values};

	{ //CREATE 
		// fixed size 'bool' with chunks: {1,1,1} enabled
		h5::create<bool>(fd,"/types/bool",{2,3,1},{1,1,1}); 
		h5::create<bool>(fd,"/types/bool unlimited",{H5S_UNLIMITED},{10}, 9 ); 
		h5::create<short>(fd,"/types/short",{2,H5S_UNLIMITED},{2,10} );
		h5::create<unsigned short>(fd,"/types/ushort",{2,3,4}  );

		h5::create<int>(fd,"/types/int",{2,3,4} );
		h5::create<unsigned int>(fd,"/types/uint",{2,3,4} );
		h5::create<long>(fd,"/types/long",{2,3,4} );
		h5::create<unsigned long>(fd,"/types/ulong",{2,3,4} );
		h5::create<long long>(fd,"/types/longlong",{2,3,4} );
		h5::create<unsigned long long>(fd,"/types/ulonglong",{2,3,4} );
		h5::create<float>(fd,"/types/float",{2,3,4} );
		h5::create<double>(fd,"/types/double",{2,3,4} );
		h5::create<std::string>(fd,"/types/string",{H5S_UNLIMITED},{10}, 3 );

		h5::create<float>(fd,"types/compressed unlimited", {4,H5S_UNLIMITED},{1,10}, 5);
		h5::create<double>(fd,"types/fixed", {4,4,10} );
	}
	/*
	 * write<T>(hid_t ds, std::initializer_list<hsize_t> offset, std::initializer_list<hsize_t> count )  
	 * when writing data from raw pointer into a HDF5, the location and size must have the same rank as 
	 * target space.
	 * */
	{
		h5::create<double>(fd,"ptr/double", {10,10} );
		h5::write<double>(fd,"ptr/double",ad, {3,1},{2,5}  ); // OK
		//h5::write<double>(fd,"ptr/double",ad, {3,1},{2,5}  );   // HDF5 RUNTIME error!!!

		double* pd = static_cast<double*>( calloc(10,sizeof(double)) );
		short*  ps = static_cast<short*>( calloc(10,sizeof(short)) );
		// reading directly into memory from opened dataset
		hid_t ds = h5::open(fd,"ptr/double");
			h5::read<double>(ds,pd,{3,1}, {2,2} );
		H5Dclose(ds);
		// read data type 'double' back into a 'short' memory space:  
		h5::read<short>(fd,"ptr/double",ps, {3,1},{2,2} );
		ds = h5::create<short>(fd,"ptr/converted to short",{10,10} );
		h5::write<short>(ds, ps, {0,0},{2,2} ); // partial write into location, with dimensions

		free( pd ); free(ps); // clean up
		H5Dclose(ds);
	}

	h5::close(fd);
}

