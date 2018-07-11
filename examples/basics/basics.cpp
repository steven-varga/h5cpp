#include <h5cpp/all>
#include <cstddef>

int main(){
	{
		h5::dcpl_t dcpl0 = h5::chunk{12} | h5::gzip{2};
		h5::dcpl_t dcpl1 = h5::chunk{12} | h5::gzip{2};
		h5::dcpl_t dcpl = dcpl0 | dcpl1;
		dcpl0 |= dcpl1;
		std::cout << sizeof( dcpl ) << " " << sizeof(hid_t) << "\n";
	}
	{ // property lists can bae daisy chained with | operator
		h5::fcpl_t fcpl = h5::file_space_page_size{4096} | h5::userblock{512};
		h5::fapl_t fapl = h5::fclose_degree_weak | h5::stdio ;
		h5::dcpl_t dcpl = h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
		h5::lcpl_t lcpl = h5::create_path | h5::utf8;
		// and come with sensible default setting: h5::default_xxxl where xxx ::= hdf5 property name 
		// h5::dapl_t dapl = h5::default_dapl; // compiler error, default values are not assignable, instead create your own as seen above
		// h5::lcpl_t lcpl = h5::create_path | h5::utf8; 
	}
	{ // all resources follow RAII idiom / managed
		h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC);  // f5::fd_t is managed resource, will call H5Fclose upon leaving code block
		hid_t ref = static_cast<hid_t>( fd ); 			    // static cast to hid_t is always allowed, ref must be treated as managed reference, 
										  				    // must not call CAPI H5Fclose( ref )  on it. This explicit or implicit conversion 
										  				    // is to support CAPI interop.   
	}
	{ // file create example: 
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

	{ // dataset create: 
		auto fd = h5::create("004.h5", H5F_ACC_TRUNC );
		auto ds_0 = h5::create<short>(fd,"/type/short/tree_0", 
				h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
				h5::create_path | h5::utf8, // optional lcpl with this default settings**
				h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9}, // optional dcpl
				h5::default_dapl ); // optional dapl
		//** lcpl controls how path (or hdf5 name: links) created, `h5::create_path` makes sure that sub paths are created  
		h5::dcpl_t dcpl = h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
		// same as above, default values implicit, dcpl explicit
		auto ds_1 = h5::create<short>(fd,"/type/short/tree_1", h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED}, dcpl);
		// same as above, default values explicit
		auto ds_2 = h5::create<short>(fd,"/type/short/tree_2", h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
				h5::default_lcpl, dcpl, h5::default_dapl);
		// if only max_dims specified, the current dims is set to max_dims or zero if the dimension is H5S_UNLIMITED
		// making it suitable storage for packet table 
		auto ds_3 = h5::create<short>(fd,"/type/short/max_dims", h5::max_dims{10,H5S_UNLIMITED}, // [10 X 0]  
			  h5::chunk{10,1} | h5::gzip{9} );
	}
}

