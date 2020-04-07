/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef H5CPP_TALL_HPP
#define H5CPP_TALL_HPP

namespace h5 {
    template <class T> using dt_t = h5::impl::detail::hid_t<T,H5Tclose,true,true,h5::impl::detail::hdf5::type>;
    template<class T> hid_t copy( const h5::dt_t<T>& dt ){
        hid_t id = static_cast<hid_t>(dt);
        H5Iinc_ref( id );
        return id;
    }
    template<class T> dt_t<T> create(){ return dt_t<T>(H5I_UNINIT); }
}

/* template specialization from hid_t< .. > type which provides syntactic sugar in the form
 * h5::dt_t<int> dt; 
 * */
namespace h5 { namespace impl { namespace detail {
    template<class T> // parent type, data_type is inherited from, see H5Iall.hpp top section for details 
    using dt_p = hid_t<T,H5Tclose,true,true,hdf5::any>;
    /*type id*/
    template<class T>
        struct hid_t<T,H5Tclose,true,true,hdf5::type> : public dt_p<T> {
        using parent = dt_p<T>;
        using hidtype = T;
        hid_t( std::initializer_list<::hid_t> fd ) : parent( fd ){}
        hid_t() : hid_t<T,H5Tclose,true,true,hdf5::any>( H5I_UNINIT ){}
    };
    template <class T> using dt_t = hid_t<T,H5Tclose,true,true,hdf5::type>;
}}}

/* template specialization is for the preceding class, and should be used only for HDF5 ELEMENT types
 * which are in C/C++ the integral types of: char,short,int,long, ... and C POD types. 
 * anything else, the ones which are considered objects/classes are broken down into integral types + container 
     * then pointer read|write is obtained to the continuous slab and delegated to h5::read | h5::write.
 * IF the data is not in a continuous memory region then it must be copied! 
 */

#define H5CPP_REGISTER_NAME( C_TYPE )                  \
template <> struct h5::name<C_TYPE> {                   \
    static constexpr char const * value = #C_TYPE;      \
};                                                      \

#define H5CPP_REGISTER_TYPE( C_TYPE, H5_TYPE )         \
template <> h5::dt_t<C_TYPE> h5::create() {             \
    h5::dt_t<C_TYPE> tid{ H5Tcopy( H5_TYPE )};          \
        if constexpr ( std::is_pointer<C_TYPE>::value ) \
            H5Tset_size (tid,H5T_VARIABLE);             \
    return tid;                                         \
};                                                      \
H5CPP_REGISTER_NAME( C_TYPE );                         \

#define H5CPP_REGISTER_VL_TYPE( C_TYPE, H5_TYPE )      \
template <> h5::dt_t<C_TYPE> h5::create() {             \
    h5::dt_t<C_TYPE> tid{                               \
       H5Tvlen_create( H5Tcopy( H5_TYPE ))};            \
    return tid;                                         \
};                                                      \
H5CPP_REGISTER_NAME( C_TYPE );                         \



#define H5CPP_REGISTER_HALF_FLOAT( C_TYPE )             \
template <> h5::dt_t<C_TYPE> create() {                 \
    h5::dt_t<C_TYPE> tid{H5Tcopy( H5T_NATIVE_FLOAT )};  \
    H5Tset_fields(tid, 15, 10, 5, 0, 10);               \
    H5Tset_precision(tid, 16);                          \
    H5Tset_ebias(tid, 15);                              \
    H5Tset_size(tid,2);                                 \
    return tid;                                         \
};                                                      \
H5CPP_REGISTER_NAME( C_TYPE );                         \

H5CPP_REGISTER_NAME( std::string );
/* registering integral data-types for NATIVE ones, which means all data is stored in the same way 
 * in file and memory: TODO: allow different types for file storage
 * */
    H5CPP_REGISTER_TYPE(bool,H5T_NATIVE_HBOOL)
    H5CPP_REGISTER_TYPE(bool*,H5T_NATIVE_HBOOL)

    H5CPP_REGISTER_TYPE(unsigned char, H5T_NATIVE_UCHAR)           H5CPP_REGISTER_TYPE(char, H5T_NATIVE_CHAR)
    H5CPP_REGISTER_TYPE(unsigned short, H5T_NATIVE_USHORT)         H5CPP_REGISTER_TYPE(short, H5T_NATIVE_SHORT)
    H5CPP_REGISTER_TYPE(unsigned int, H5T_NATIVE_UINT)             H5CPP_REGISTER_TYPE(int, H5T_NATIVE_INT)
    H5CPP_REGISTER_TYPE(unsigned long int, H5T_NATIVE_ULONG)       H5CPP_REGISTER_TYPE(long int, H5T_NATIVE_LONG)
    H5CPP_REGISTER_TYPE(unsigned long long int, H5T_NATIVE_ULLONG) H5CPP_REGISTER_TYPE(long long int, H5T_NATIVE_LLONG)
    H5CPP_REGISTER_TYPE(float, H5T_NATIVE_FLOAT)                   H5CPP_REGISTER_TYPE(double, H5T_NATIVE_DOUBLE)
    H5CPP_REGISTER_TYPE(long double,H5T_NATIVE_LDOUBLE)

    H5CPP_REGISTER_TYPE(unsigned char*, H5T_C_S1)                  H5CPP_REGISTER_TYPE(char*, H5T_C_S1)

    H5CPP_REGISTER_VL_TYPE(unsigned short*, H5T_NATIVE_USHORT)         H5CPP_REGISTER_VL_TYPE(short*, H5T_NATIVE_SHORT)
    H5CPP_REGISTER_VL_TYPE(unsigned int*, H5T_NATIVE_UINT)             H5CPP_REGISTER_VL_TYPE(int*, H5T_NATIVE_INT)
    H5CPP_REGISTER_VL_TYPE(unsigned long int*, H5T_NATIVE_ULONG)       H5CPP_REGISTER_VL_TYPE(long int*, H5T_NATIVE_LONG)
    H5CPP_REGISTER_VL_TYPE(unsigned long long int*, H5T_NATIVE_ULLONG) H5CPP_REGISTER_VL_TYPE(long long int*, H5T_NATIVE_LLONG)
    H5CPP_REGISTER_VL_TYPE(float*, H5T_NATIVE_FLOAT)                   H5CPP_REGISTER_VL_TYPE(double*, H5T_NATIVE_DOUBLE)
    H5CPP_REGISTER_VL_TYPE(long double*,H5T_NATIVE_LDOUBLE)

// half float support: 
// TODO: factor out in a separate file
#ifdef HALF_HALF_HPP
    H5CPP_REGISTER_HALF_FLOAT(half_float::half)
#endif
// Open XDR doesn-t define namespace or 
#ifdef WITH_OPENEXR_HALF 
    H5CPP_REGISTER_HALF_FLOAT(OPENEXR_NAMESPACE::half)
}
#endif
#define H5CPP_REGISTER_STRUCT( POD_STRUCT ) 

namespace h5 {
    template <> struct name<char**> { static constexpr char const * value = "char**"; };
    template <> struct name<short**> { static constexpr char const * value = "short**"; };
    template <> struct name<int**> { static constexpr char const * value = "int**"; };
    template <> struct name<long**> { static constexpr char const * value = "long**"; };
    template <> struct name<long long **> { static constexpr char const * value = "long long**"; };
    template <> struct name<float**> { static constexpr char const * value = "float**"; };
    template <> struct name<double**> { static constexpr char const * value = "double**"; };
    template <> struct name<void**> { static constexpr char const * value = "void**"; };
    template <class T> struct name<std::complex<T>> { static constexpr char const * value = "complex<T>"; };
    template <> struct name<std::complex<float>> { static constexpr char const * value = "complex<float>"; };
    template <> struct name<std::complex<double>> { static constexpr char const * value = "complex<double>"; };
    template <> struct name<std::complex<short>> { static constexpr char const * value = "complex<short>"; };
    template <> struct name<std::complex<int>> { static constexpr char const * value = "complex<int>"; };
}
#endif
