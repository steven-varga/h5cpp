/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef H5CPP_TALL_HPP
#define H5CPP_TALL_HPP

namespace h5 {
    // variable length data struct
    template <class T> struct vlen_t {
        operator hvl_t() const {
            return *this;
        }
        size_t len;
        T* ptr;
    };
    template <class T> using dt_t = h5::impl::detail::hid_t<T,H5Tclose,true,true,h5::impl::detail::hdf5::type>;
    template<class T> hid_t copy( const h5::dt_t<T>& dt ){
        hid_t id = static_cast<hid_t>(dt);
        H5Iinc_ref( id );
        return id;
    }
    template<class T> dt_t<T> copy();
    template<class T> dt_t<T> create();
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

        template <class K, int N>
        operator hid_t<K[N],H5Tclose,true,true,hdf5::type>() const {
            if( H5Iis_valid( this->handle ) )
				H5Iinc_ref( this->handle );
            return hid_t<K[N],H5Tclose,true,true,hdf5::type>{this->handle};
        }
        template <class K>
        operator hid_t<K,H5Tclose,true,true,hdf5::type>() const {
            if( H5Iis_valid( this->handle ) )
				H5Iinc_ref( this->handle );
            return hid_t<K,H5Tclose,true,true,hdf5::type>{this->handle};
        }
    };
    template <class T> using dt_t = hid_t<T,H5Tclose,true,true,hdf5::type>;
}}}

/* template specialization is for the preceding class, and should be used only for HDF5 ELEMENT types
 * which are in C/C++ the integral types of: char,short,int,long, ... and C POD types. 
 * anything else, the ones which are considered objects/classes are broken down into integral types + container 
     * then pointer read|write is obtained to the continuous slab and delegated to h5::read | h5::write.
 * IF the data is not in a continuous memory region then it must be copied! 
 */

#define H5CPP_REGISTER_NAME( C_TYPE )                   \
template <> struct h5::name<C_TYPE> {                   \
    static constexpr char const * value = #C_TYPE;      \
};                                                      \

#define H5CPP_REGISTER_TYPE( C_TYPE, H5_TYPE )          \
template <> h5::dt_t<C_TYPE> h5::copy() {               \
    h5::dt_t<C_TYPE> tid{ H5Tcopy( H5_TYPE )};          \
    return tid;                                         \
};                                                      \
H5CPP_REGISTER_NAME( C_TYPE  );                         \

#define H5CPP_REGISTER_HALF_FLOAT( C_TYPE )             \
template <> h5::dt_t<C_TYPE> create() {                 \
    h5::dt_t<C_TYPE> tid{H5Tcopy( H5T_NATIVE_FLOAT )};  \
    H5Tset_fields(tid, 15, 10, 5, 0, 10);               \
    H5Tset_precision(tid, 16);                          \
    H5Tset_ebias(tid, 15);                              \
    H5Tset_size(tid,2);                                 \
    return tid;                                         \
};                                                      \
H5CPP_REGISTER_NAME( C_TYPE );                          \

#define H5CPP_REGISTER_VL_STRING( C_TYPE )              \
    template <> h5::dt_t<C_TYPE> h5::create() {         \
        h5::dt_t<std::string> tid{H5Tcopy( H5T_C_S1 )}; \
        H5Tset_size(tid,H5T_VARIABLE);                  \
        H5Tset_cset(tid, H5T_CSET_UTF8);                \
        return tid;                                     \
    };                                                  \
H5CPP_REGISTER_NAME( C_TYPE );                          \

#define H5CPP_REGISTER_ARRAY( C_TYPE )                  \
    template <> h5::dt_t<C_TYPE> h5::create() {         \
        return tid;                                     \
    };                                                  \
H5CPP_REGISTER_NAME( C_TYPE );                          \



//H5CPP_REGISTER_NAME( std::string );
/* registering integral data-types for NATIVE ones, which means all data is stored in the same way 
 * in file and memory: TODO: allow different types for file storage
 * */
    H5CPP_REGISTER_TYPE(bool,H5T_NATIVE_HBOOL)                     H5CPP_REGISTER_TYPE(std::byte, H5T_NATIVE_UCHAR) 
    H5CPP_REGISTER_TYPE(signed char, H5T_NATIVE_CHAR)
    H5CPP_REGISTER_TYPE(unsigned char, H5T_NATIVE_UCHAR)           H5CPP_REGISTER_TYPE(char, H5T_NATIVE_CHAR)
    H5CPP_REGISTER_TYPE(unsigned short, H5T_NATIVE_USHORT)         H5CPP_REGISTER_TYPE(short, H5T_NATIVE_SHORT)
    H5CPP_REGISTER_TYPE(unsigned int, H5T_NATIVE_UINT)             H5CPP_REGISTER_TYPE(int, H5T_NATIVE_INT)
    H5CPP_REGISTER_TYPE(unsigned long int, H5T_NATIVE_ULONG)       H5CPP_REGISTER_TYPE(long int, H5T_NATIVE_LONG)
    H5CPP_REGISTER_TYPE(unsigned long long int, H5T_NATIVE_ULLONG) H5CPP_REGISTER_TYPE(long long int, H5T_NATIVE_LLONG)
    //Gerd Heber conversation 2020, jun 29:  H5CPP_REGISTER_TYPE(size_t, H5T_NATIVE_HSIZE)

    H5CPP_REGISTER_TYPE(float, H5T_NATIVE_FLOAT)                   H5CPP_REGISTER_TYPE(double, H5T_NATIVE_DOUBLE)
    H5CPP_REGISTER_TYPE(long double,H5T_NATIVE_LDOUBLE)

    H5CPP_REGISTER_VL_STRING(std::string)     H5CPP_REGISTER_VL_STRING(char**)
    H5CPP_REGISTER_VL_STRING(const char**)     H5CPP_REGISTER_VL_STRING(const char* const*)

namespace h5::stl {
    template <class T, class...>
        h5::dt_t<T> copy(){
            return h5::dt_t<T>{H5Tcopy(H5T_NATIVE_INT)};
        }
}

    // template specialization 
    template<class T> h5::dt_t<T> h5::create() {
        using ptr_t = typename impl::decay<T>::type;
        using element_t = typename std::remove_pointer<ptr_t>::type;
        h5::dt_t<T> tid;
        if constexpr( impl::is_array<T>::value ) { // T[]
            if constexpr(std::is_same<char,ptr_t>::value){ // char[] is mapped to fixed size string
                constexpr size_t rank = std::rank<T>::value;
                constexpr std::array<hsize_t,rank> size = h5::meta::get_extent<T,rank>();

                tid = h5::dt_t<char>{H5Tcopy(H5T_C_S1)};
                H5Tset_cset(tid, H5T_CSET_UTF8);
                H5Tset_size (tid, size[rank-1]);
                if constexpr( rank > 1 )
                    tid = h5::dt_t<T>{ H5Tarray_create(tid, rank-1, size.data())};
            } else if constexpr ( std::is_pointer<ptr_t>::value ){
                // array of pointers -> VL string array
                if constexpr( std::is_same<ptr_t, char const*>::value ){
                    constexpr size_t rank = std::rank<T>::value;
                    constexpr std::array<hsize_t,rank> size = h5::meta::get_extent<T,rank>();

                    auto id = h5::create<std::string>();
                    tid = h5::dt_t<T>{ H5Tarray_create(id, rank, size.data()) };
                } else
                    static_assert(true, "variable length arrays are not implemented yet...");
            } else { // T[] but not char-s are mapped to array types
                using E = typename impl::decay<T>::type;
                h5::dt_t<E> id = h5::create<E>();
                // FIXME: std::array extent is not computed right
                if constexpr(impl::is_stl<T>::value) {
                    constexpr hsize_t N=std::tuple_size<T>::value;
                    tid = h5::dt_t<T>{ H5Tarray_create(id, 1, &N)};
                } else {
                    constexpr size_t rank = std::rank<T>::value;
                    constexpr std::array<hsize_t,rank> size = h5::meta::get_extent<T,rank>();
                    tid = h5::dt_t<T>{ H5Tarray_create(id, rank, size.data()) };
                }
            }
        } else {  // elementary types<float,double, char, short, int ... > are handled below
            if constexpr( std::is_same<T, char const*>::value )
                tid = h5::create<std::string>();
            else if constexpr(impl::is_stl<T>::value)
                tid = h5::create<element_t>();
            else tid = h5::copy<T>();
        }
        return tid;
    }

namespace h5 {
    template <class T, int N> struct name<T[N]> {
        static constexpr const char* value = meta::extent_to_string<T[N]>::value;
    };
}
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
    template <> struct name<char const*> { static constexpr char const * value = "char const*"; };
    //template <> struct name<char**> { static constexpr char const * value = "char**"; };
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
