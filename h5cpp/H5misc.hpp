/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_MISC_HPP
#define  H5CPP_MISC_HPP
namespace h5{
	using cx_double =  std::complex<double>; /**< scientific type */
	using cx_float = std::complex<float>;    /**< scientific type */
	using herr_t = ::herr_t;

	static thread_local herr_t (*error_stack_callback)(::hid_t, void*);
	static thread_local void * error_stack_client_data;
}

namespace h5 { namespace utils {
	inline static herr_t h5_callback( hid_t g_id, const char *name, 
			const H5L_info_t *info, void *op_data){

		std::vector<std::string> *data =  static_cast<std::vector<std::string>* >(op_data); 
		data->push_back( std::string(name) );
		return 0;
	}
/*
    inline std::vector<std::string> list_directory(const h5::fd_t& fd,  const std::string& directory ){
        hid_t group_id = H5Gopen( static_cast<hid_t>(fd), directory.c_str(), H5P_DEFAULT );
        std::vector<std::string> files;
            H5Literate( group_id, H5_INDEX_NAME, H5_ITER_INC, 0, &h5_callback, &files );
		H5Gclose(group_id);
        return files;
    }*/
}}


namespace h5{
	/** \ingroup file-io  
	 * A dummy call, to facilitate open-do some work - close pattern
	 * \par_fd \sa_h5cpp \sa_hdf5 \sa_stl 
	 * \code 
	 *   h5::fd_t fd = h5::open("example.h5", H5F_ACC_RDWR);  	// returned descriptor automatically 
	 *      ... 												// do some work
	 *   h5::close(fd); 										// dummy call for symmetry
	 * \endcode
	 */
	//inline void close(h5::fd_t& fd) {
	//}

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

#define H5CPP_supported_elementary_types "supported elementary types ::= pod_struct | float | double |  [signed](int8 | int16 | int32 | int64)"

namespace h5 { namespace utils {
	template <class T>
	static constexpr bool is_supported = std::is_class<T>::value | std::is_arithmetic<T>::value;
	//static constexpr bool is_supported = std::is_pod<T>::value && std::is_class<T>::value | std::is_arithmetic<T>::value;
}}




namespace h5 { namespace utils {

	template <typename T> inline  std::vector<T> get_test_data( size_t n ){
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::uniform_int_distribution<> dist(0,n);

		std::vector<T> data;
		data.reserve(n);
		std::generate_n(std::back_inserter(data), n, [&]() {
								return dist(rng);
							});
		return data;
	}
// TODO: bang this so total memory alloc is same as 'n'
	template <> inline std::vector<std::string> get_test_data( size_t n ){

		std::vector<std::string> data;
		data.reserve(n);

		static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
										"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
		std::random_device rd;
		std::default_random_engine rng(rd());
		std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);
		std::uniform_int_distribution<> string_length(5,30);

		std::generate_n(std::back_inserter(data), data.capacity(),   [&] {
				std::string str;
				size_t N = string_length(rng);
				str.reserve(N);
				std::generate_n(std::back_inserter(str), N, [&]() {
								return alphabet[dist(rng)];
							});
				  return str;
				  });
		return data;
	}
}}
#endif

