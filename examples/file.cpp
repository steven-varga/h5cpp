#include <h5cpp/all>
#include <cstddef>

#define values 0,1,2,3,4,5,6,7,8,9

int main(){
	{
		h5::fcpl_t fcpl = h5::userblock{512};
		h5::dcpl_t dcpl =  h5::fill_value<short>{42} | h5::gzip{9};
	}
	{
		h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC);  // f5::fd_t is managed resource, will call H5Fclose upon leaving code block
		hid_t ref = static_cast<hid_t>( fd ); 			    // static cast to hid_t is always allowed, ref must be treated as managed reference, 
										  				    // must not call CAPI H5Fclose( ref )  on it. This explicit or implicit conversion 
										  				    // is to support CAPI interop.   
	} // closes fd when leaving this scope
	{
		// flags := H5F_ACC_TRUNC | H5F_ACC_EXCL either to truncate or open file exclusively
		// you may pass CAPI property list descriptors daisy chained with '|' operator 
		auto fd = h5::create("002.h5", H5F_ACC_TRUNC, 
				h5::file_space_page_size{4096} | h5::userblock{512},  // file creation properties
			   	h5::fclose_degree_weak | h5::fapl_core{2048,1} );     // file access properties
		// or the c++11 wrapped smart pointer equivalent h5::AP_DEFAULT
		h5::create("003.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
		// file is closed when returning h5::create function, convenient if we don't proceed with
		// creating dataset
	}

	{ //CREATE
		auto fd = h5::create("002.h5", H5F_ACC_TRUNC );
		//h5::fapl_t fapl = h5::fclose_degree_weak | h5::stdio ;
		h5::dcpl_t dcpl = h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
		// create an extendable 2D array, and get a handle to the object that 
		// automatically closes resource when leaving code block. Or call
		// return std::move(ds ) to take it to outer scope.  

		auto ds = h5::create<short>(fd,"/type/short/tree",{2,H5S_UNLIMITED},
			   h5::create_path | h5::utf8, dcpl );

		// this is also a valid construct of a fixed size 3D array, signaling that the handle won't be used
		// further, underlying resource is closed as expected. By default  -DH5CPP_MAX_RANK=7 defines maximum
		// dimensions of an array, values maybe in the range of 1-11 
		//h5::create<unsigned short>(fd,"/types/ushort",{2,3,4}   );

		// it works on std::string as well
		//h5::create<std::string>(fd,"/types/string",{H5S_UNLIMITED},{10}, 3 );
		// dataset maybe  compressed individually, compression applied to chunks, in other words
		// only the part is uncompressed which you operate on, then if modified, compressed back
		//h5::create<float>(fd,"types/compressed unlimited", {4,H5S_UNLIMITED},{1,10}, 5);

		// if RAII not enforced, hid_t types can also be passed transparently where it makes
		// sense:
	//	hid_t ref = static_cast<hid_t>(fd);
		//h5::create<float>(ref,"function call with hid_t type", {4,H5S_UNLIMITED},{1,10}, 5);
	}

} 

