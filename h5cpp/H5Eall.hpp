/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */


#ifndef  H5CPP_EALL_HPP
#define  H5CPP_EALL_HPP
namespace h5{
	using herr_t = ::herr_t;

	static thread_local herr_t (*error_stack_callback)(::hid_t, void*);
	static thread_local void * error_stack_client_data;
}

namespace h5 {
	/**  @ingroup file-io
	 * @brief removes default error handler	preventing diagnostic error messages printed
	 * for direct CAPI calls. This is a thread safe implementation. [Read on Error Handling/Exceptions](@ref link_error_handler)
	 * \sa_h5cpp \sa_hdf5 \sa_stl 
	 * @code
	 * h5::mute();             // mute  error handling
	 *    ... H5?xxx calls ... // do your capi calls which output annoying error messages
	 * h5::unmute();           // restore previously saved handler
	 * @endcode
	 */
    inline void mute( ){
		H5Eget_auto2(H5E_DEFAULT, &error_stack_callback, &error_stack_client_data);
		H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    }
	/**  @ingroup file-io
	 * @brief restores previously saved error handler with h5::mute [Read on Error Handling/Exceptions](@ref link_error_handler)
	 * @code
	 * @code
	 * h5::mute();             // mute  error handling
	 *    ... H5?xxx calls ... // do your capi calls which output annoying error messages
	 * h5::unmute();           // restore previously saved handler
	 * @endcode
	 */
    inline void unmute( ){
		H5Eset_auto2(H5E_DEFAULT, error_stack_callback, error_stack_client_data);
    }
}

namespace h5 { namespace error {
	struct any : public std::runtime_error {
		any() : std::runtime_error("H5CPP ERROR") {}
		any(const std::string& msg ) : std::runtime_error( msg ){}
	};
	struct rollback {
		rollback(const std::string& msg ) {
		std::cerr <<"UNRECOVERABLE ERROR:" <<  msg << std::endl;
#ifdef H5CPP_HARD_ERROR
	exit(1)
#endif
		}
	};
	namespace msg {
		const std::string inc_ref   = "couldn't increment reference...";
		const std::string dec_ref   = "couldn't decrement reference...";
		const std::string create_memspace = "couldn't create memory space...";
		const std::string select_memspace = "couldn't do  memory space selection...";
		const std::string select_hyperslab = "couldn't do hyper-slab  selection...";
		const std::string get_filespace = "couldn't get file space...";
		const std::string get_memspace = "couldn't get memory space...";
		const std::string get_memtype = "couldn't get mem type...";
		const std::string get_filetype = "couldn't get file type...";
		const std::string get_filetype_size = "could_t get file type size...";
		const std::string close_memspace = "could_t close memory space...";
		const std::string close_filespace = "could_t close file space...";
		const std::string close_filetype = "could_t close file type...";
		const std::string close_memtype = "could_t close memory type...";
		const std::string close_property_list = "couldn't close property list...";
		const std::string close_group = "couldn't close group/directory...";
		const std::string get_rank = "couldn't obtain rank...";
		const std::string get_chunk_dims = "couldn't obtain chunk dimensions...";
		const std::string get_dims = "couldn't obtain data dimensions...";
		const std::string mem_alloc = "couldn't allocate memory of requested size...";
		const std::string set_extent = "couldn't set extent...";
		const std::string set_chunk = "couldn't set chunk...";
		const std::string write_dataset = "couldn't write dataset...";
		const std::string read_dataset = "couldn't read dataset...";
		const std::string create_dataset = "couldn't create dataset...";
		const std::string create_file = "couldn't create file...";
		const std::string create_property_list = "couldn't create property list...";
		const std::string create_dims = "couldn't create dimension descriptor...";
		const std::string link_check = "checking if object exists fails...";
		const std::string open_dataset = "opening dataset failed...";
		const std::string open_file = "opening hdf5 container failed...";
		const std::string open_group = "opening directory failed...";
		const std::string dataset_descriptor = "invalid dataset descriptor ...";
		const std::string file_descriptor = "invalid file descriptor ...";
		const std::string file_space = "invalid file space ...";
		const std::string prop_descriptor = "invalid file descriptor ...";
		const std::string list_directory = "error traversing directory ...";
		const std::string rank_mismatch = "rank of file space and mem space must match ...";
		const std::string get_simple_extent_dims = "???? ...";
		const std::string get_dataset_type = "???? ...";
		const std::string create_dcpl = "???? ...";
	}
}}
namespace h5 { namespace error { namespace io {
	struct rollback : public h5::error::rollback {
		rollback( const std::string& msg ) : h5::error::rollback( msg ){}
	};
	struct any : public h5::error::any {
		any() : h5::error::any() {}
		any( const std::string& msg ) : h5::error::any( msg ){}
	};
}}}

namespace h5 { namespace error { namespace io { namespace file {
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	struct open : public h5::error::io::file::any {
		open() : h5::error::io::file::any() {}
		open( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	struct close : public h5::error::io::file::any {
		close() : h5::error::io::file::any() {}
		close( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	struct read : public h5::error::io::file::any {
		read() : h5::error::io::file::any() {}
		read( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	struct write : public h5::error::io::file::any {
		write() : h5::error::io::file::any() {}
		write( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	struct create : public h5::error::io::file::any {
		create() : h5::error::io::file::any() {}
		create( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
	struct misc : public h5::error::io::file::any {
		misc() : h5::error::io::file::any() {}
		misc( const std::string& msg ) : h5::error::io::file::any( msg ){}
	};
}}}}

namespace h5 { namespace error { namespace io { namespace dataset {
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	struct open : public h5::error::io::dataset::any {
		open() : h5::error::io::dataset::any() {}
		open( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	struct close : public h5::error::io::dataset::any {
		close() : h5::error::io::dataset::any() {}
		close( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	struct read : public h5::error::io::dataset::any {
		read() : h5::error::io::dataset::any() {}
		read( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	struct write : public h5::error::io::dataset::any {
		write() : h5::error::io::dataset::any() {}
		write( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	struct append : public h5::error::io::dataset::any {
		append() : h5::error::io::dataset::any() {}
		append( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	struct create : public h5::error::io::dataset::any {
		create() : h5::error::io::dataset::any() {}
		create( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
	struct misc : public h5::error::io::dataset::any {
		misc() : h5::error::io::dataset::any() {}
		misc( const std::string& msg ) : h5::error::io::dataset::any( msg ){}
	};
}}}}
namespace h5 { namespace error { namespace io { namespace packet_table {
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	struct open : public h5::error::io::packet_table::any {
		open() : h5::error::io::packet_table::any() {}
		open( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	struct close : public h5::error::io::packet_table::any {
		close() : h5::error::io::packet_table::any() {}
		close( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	struct read : public h5::error::io::packet_table::any {
		read() : h5::error::io::packet_table::any() {}
		read( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	struct write : public h5::error::io::packet_table::any {
		write() : h5::error::io::packet_table::any() {}
		write( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	struct append : public h5::error::io::packet_table::any {
		append() : h5::error::io::packet_table::any() {}
		append( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	struct create : public h5::error::io::packet_table::any {
		create() : h5::error::io::packet_table::any() {}
		create( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
	struct misc : public h5::error::io::packet_table::any {
		misc() : h5::error::io::packet_table::any() {}
		misc( const std::string& msg ) : h5::error::io::packet_table::any( msg ){}
	};
}}}}

namespace h5 { namespace error { namespace io { namespace attribute {
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	struct open : public h5::error::io::attribute::any {
		open() : h5::error::io::attribute::any() {}
		open( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	struct close : public h5::error::io::attribute::any {
		close() : h5::error::io::attribute::any() {}
		close( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	struct read : public h5::error::io::attribute::any {
		read() : h5::error::io::attribute::any() {}
		read( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	struct write : public h5::error::io::attribute::any {
		write() : h5::error::io::attribute::any() {}
		write( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	struct create : public h5::error::io::attribute::any {
		create() : h5::error::io::attribute::any() {}
		create( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
	struct misc : public h5::error::io::attribute::any {
		misc() : h5::error::io::attribute::any() {}
		misc( const std::string& msg ) : h5::error::io::attribute::any( msg ){}
	};
}}}}

namespace h5 { namespace error { namespace property_list {
	struct rollback : public h5::error::rollback {
		rollback( const std::string& msg ) : h5::error::rollback( msg ){}
	};
	struct any : public h5::error::any {
		any() : h5::error::any() {}
		any( const std::string& msg ) : h5::error::any( msg ){}
	};
	struct argument : public h5::error::property_list::any {
		argument() : h5::error::property_list::any() {}
		argument( const std::string& msg ) : h5::error::property_list::any( msg ){}
	};
	struct misc : public h5::error::property_list::any {
		misc() : h5::error::property_list::any() {}
		misc( const std::string& msg ) : h5::error::property_list::any( msg ){}
	};
}}}
#endif

