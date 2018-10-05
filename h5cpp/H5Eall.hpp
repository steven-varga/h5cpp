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

namespace h5 { namespace error { namespace io { namespace property_list {
	struct rollback : public h5::error::io::rollback {
		rollback( const std::string& msg ) : h5::error::io::rollback( msg ){}
	};
	struct any : public h5::error::io::any {
		any() : h5::error::io::any() {}
		any( const std::string& msg ) : h5::error::io::any( msg ){}
	};
	struct open : public h5::error::io::property_list::any {
		open() : h5::error::io::property_list::any() {}
		open( const std::string& msg ) : h5::error::io::property_list::any( msg ){}
	};
	struct close : public h5::error::io::property_list::any {
		close() : h5::error::io::property_list::any() {}
		close( const std::string& msg ) : h5::error::io::property_list::any( msg ){}
	};
	struct read : public h5::error::io::property_list::any {
		read() : h5::error::io::property_list::any() {}
		read( const std::string& msg ) : h5::error::io::property_list::any( msg ){}
	};
	struct write : public h5::error::io::property_list::any {
		write() : h5::error::io::property_list::any() {}
		write( const std::string& msg ) : h5::error::io::property_list::any( msg ){}
	};
	struct create : public h5::error::io::property_list::any {
		create() : h5::error::io::property_list::any() {}
		create( const std::string& msg ) : h5::error::io::property_list::any( msg ){}
	};
	struct misc : public h5::error::io::property_list::any {
		misc() : h5::error::io::property_list::any() {}
		misc( const std::string& msg ) : h5::error::io::property_list::any( msg ){}
	};
}}}}
#endif

