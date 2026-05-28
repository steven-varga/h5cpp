// Copyright (c) 2018-2026 Steven Varga, Toronto, ON Canada

#include <h5cpp/all>
#include <cstddef>
#include <numeric>

int main(){
	// Data types are mapped from the C/C++ type system to HDF5 through class templates.
	// h5::dt_t<T> creates an HDF5 type descriptor for T.
	{
		// The default constructor selects H5T_NATIVE_INT through template partial
		// specialization, then obtains an hid_t HDF5 type descriptor via H5Tcopy.
		// Through template inheritance its behaviour matches the rest of the descriptors.
		// RAII closes the resource when leaving scope.
		h5::dt_t<int> my_int_type;

		// Depending on the conversion policy, the type id may be implicitly or explicitly
		// cast to a C-API-style HDF5 ID.
		hid_t capi_style_id = static_cast<hid_t>( my_int_type );

		// For types not yet defined, register them with the following macro:
		// H5CPP_REGISTER_TYPE(C_COMPOUND_TYPE, HDF5_COMPOUND_TYPE)
		// This is a template specialization for the given type; see H5Tall.hpp for details.
		//
		H5CPP_CHECK_EQ( H5Tequal(capi_style_id, H5T_NATIVE_INT),
				std::runtime_error, "HDF5 type system failure!!!" )

		// Types have names at compile time and runtime.
		std::cout << h5::name<int>::value << std::endl; // compile-time type name
		std::cout << my_int_type << std::endl;          // runtime type information
	}
	{
		h5::dcpl_t dcpl0 = h5::chunk{12} | h5::gzip{2};
		h5::dcpl_t dcpl1 = h5::chunk{12} | h5::gzip{2};
		h5::dcpl_t dcpl = dcpl0 | dcpl1;
		dcpl0 |= dcpl1;
	}
	// error handling
	{
		h5::mute(); // suppress CAPI error messages for expected failures
		try {
			h5::dcpl_t dcpl_0 = h5::gzip{79798}; // invalid argument
		} catch ( const h5::error::any& err ){
			std::cerr << "THIS ERROR IS INTENTIONAL: " << err.what() << std::endl;
		}
		h5::unmute();
	}

	{ // Property lists can be daisy-chained with the | operator.
		h5::fcpl_t fcpl = h5::file_space_page_size{4096} | h5::userblock{512};
		h5::fapl_t fapl = h5::fclose_degree_weak | h5::stdio;
		auto some_prop = h5::libver_bounds({H5F_LIBVER_LATEST, H5F_LIBVER_LATEST});
		(void)some_prop; // libver_bounds returns an object, keep it alive if used
		h5::dcpl_t dcpl = h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
		h5::lcpl_t lcpl = h5::create_path | h5::utf8;
		// Defaults are available as h5::default_xxxl where xxx is the HDF5 property name.
		// h5::dapl_t dapl = h5::default_dapl; // compiler error: default values are not assignable.
		// Instead, create your own as shown above.
	}
	{ // All resources follow the RAII idiom.
		h5::fd_t fd = h5::create("001.h5", H5F_ACC_TRUNC);  // h5::fd_t is a managed resource; H5Fclose is called on scope exit.
		hid_t ref = static_cast<hid_t>( fd );               // static_cast to hid_t is always allowed. Treat ref as a borrowed
		(void)ref;                                          // reference — do not call H5Fclose(ref) manually.
		// This explicit or implicit conversion exists to support CAPI interop.
	}
	{ // file creation example:
		// flags := H5F_ACC_TRUNC | H5F_ACC_EXCL either truncate or open the file exclusively.
		// CAPI property list descriptors can be daisy-chained with the | operator.
		auto fd = h5::create("002.h5", H5F_ACC_TRUNC,
				h5::file_space_page_size{4096} | h5::userblock{512} );  // file creation properties
			    	//,h5::fclose_degree_weak | h5::fapl_core{2048,1} );  // file access properties
		// Or use the raw CAPI default property lists:
		h5::create("003.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT );
		// The file is closed when h5::create returns, which is convenient if we do not
		// proceed with creating a dataset.
	}

	{ // dataset creation:
		auto fd = h5::create("004.h5", H5F_ACC_TRUNC );
		auto ds_0 = h5::create<short>(fd,"/type/short/tree_0",
				h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
				h5::create_path | h5::utf8, // optional lcpl; default settings are shown above**
				h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9}, // optional dcpl
				h5::default_dapl ); // optional dapl
		// ** lcpl controls how paths (or HDF5 names: links) are created. h5::create_path ensures
		//    that missing intermediate groups are created automatically.
		h5::dcpl_t dcpl = h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{2};
		// same as above, defaults implicit, dcpl explicit
		auto ds_1 = h5::create<short>(fd,"/type/short/tree_1", h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED}, dcpl);
		// same as above, defaults explicit
		auto ds_2 = h5::create<short>(fd,"/type/short/tree_2", h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
				h5::default_lcpl, dcpl, h5::default_dapl);
		// If only max_dims is specified, current_dims is set to max_dims or to zero when the
		// dimension is H5S_UNLIMITED, making it suitable for packet-table-style storage.
		// gzip{0} means the lowest level of compression, not "no compression".
		auto ds_3 = h5::create<short>(fd,"/type/short/max_dims", h5::max_dims{10,H5S_UNLIMITED}, // logical shape: [10 x 0]
			  h5::chunk{10,1}  );
	}

	{ // writing and reading data:
		auto fd = h5::create("005.h5", H5F_ACC_TRUNC );
		// In the C API you would create a dataspace, a type, a property list, call H5Dcreate,
		// then H5Dwrite. In H5CPP the common path is a single call.
		std::vector<double> data(100);
		std::iota(data.begin(), data.end(), 0.0);

		// one-shot create + write
		h5::write(fd, "/dataset", data, h5::chunk{10} | h5::gzip{2});

		// reading back: H5CPP infers the memory space and allocates the container
		auto result = h5::read<std::vector<double>>(fd, "/dataset");
	}

}
