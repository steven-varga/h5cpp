/*
 * Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */
#pragma once
#include <hdf5.h>
#include "H5meta.hpp"
#include "H5Iall.hpp"
#include "H5Tmeta.hpp"
#include <type_traits>
#include <ostream>
#include <optional>
#include <initializer_list>
#include <complex>
#include <utility>

namespace h5 {
    template<class T> hid_t register_struct(){ return H5I_UNINIT; }
	struct reference_t {
#if H5_VERSION_GE(1,12,0)
        H5R_ref_t value{}; //< HDF5 1.12+ generic reference storage; zero-init means "empty"

        // RAII: every successful H5Rcreate_*/H5Rcopy must be matched with H5Rdestroy.
        // Without these, std::vector<reference_t> and bare scalar references leak the
        // internal handle each H5R_ref_t carries, and HDF5 hits its "infinite loop
        // closing library" path at shutdown (the leaked handles keep parents alive).
        // The opaque H5R_ref_t blob remains memcpy-safe for HDF5's on-disk serializer
        // (H5Dread/H5Dwrite go through H5T_STD_REF), which is what `is_trivially_packable`
        // in access_traits_t<reference_t> asserts. The rule-of-five below governs only
        // *in-memory* C++ copy/move; the disk path is unaffected.
        reference_t() noexcept = default;
        ~reference_t() {
            if (!is_empty()) H5Rdestroy(&value);
        }
        reference_t(const reference_t& other) {
            if (!other.is_empty()) H5Rcopy(&other.value, &value);
        }
        reference_t& operator=(const reference_t& other) {
            if (this != &other) {
                if (!is_empty()) H5Rdestroy(&value);
                std::memset(&value, 0, sizeof(value));
                if (!other.is_empty()) H5Rcopy(&other.value, &value);
            }
            return *this;
        }
        reference_t(reference_t&& other) noexcept {
            std::memcpy(&value, &other.value, sizeof(value));
            std::memset(&other.value, 0, sizeof(other.value));
        }
        reference_t& operator=(reference_t&& other) noexcept {
            if (this != &other) {
                if (!is_empty()) H5Rdestroy(&value);
                std::memcpy(&value, &other.value, sizeof(value));
                std::memset(&other.value, 0, sizeof(other.value));
            }
            return *this;
        }
    private:
        bool is_empty() const noexcept {
            // A value-initialized H5R_ref_t is all-zero bytes; H5Rdestroy on it
            // would log an HDF5 error. memcmp against a static zero blob is the
            // cheapest portable check.
            static constexpr H5R_ref_t zero{};
            return std::memcmp(&value, &zero, sizeof(value)) == 0;
        }
    public:
#else
        hdset_reg_ref_t value{}; //< region or object ref storage (pre-1.12 POD, no handle)
#endif
    };
#if H5_VERSION_GE(1,12,0)
    static_assert(sizeof(reference_t) == sizeof(H5R_ref_t), "reference_t must match HDF5 reference storage");
    static_assert(std::is_nothrow_move_constructible<reference_t>::value,
        "reference_t move must be noexcept so std::vector reallocation uses move, not copy");
#else
    static_assert(sizeof(reference_t) == sizeof(hdset_reg_ref_t), "reference_t must match HDF5 dataset-region reference storage");
#endif
}

namespace h5::meta {
    // h5::reference_t is a POD scalar — single HDF5 reference value
    template <>
    struct access_traits_t<h5::reference_t> {
        using element_t  = h5::reference_t;
        using pointer_t  = const h5::reference_t*;
        static constexpr access_t kind = access_t::object;
        static constexpr bool is_trivially_packable = true;
        static const h5::reference_t* data(const h5::reference_t& v) noexcept { return &v; }
        static h5::reference_t*       data(h5::reference_t& v)       noexcept { return &v; }
        static constexpr std::array<std::size_t,0> size(const h5::reference_t&) noexcept { return {}; }
        static constexpr std::size_t bytes(const h5::reference_t&) noexcept { return sizeof(h5::reference_t); }
    };
}

// Explicit storage_representation so the H5Dwrite / H5Dread `storage != unsupported`
// static_assert passes for h5::reference_t (scalar) and for std::vector<reference_t>
// (linear_value_dataset of scalar). Mirrors the std::tuple / std::pair / std::complex
// registrations at H5Tmeta.hpp:218-248.
namespace h5::meta::detail_capabilities {
    template <> struct has_explicit_storage_repr<h5::reference_t> : std::true_type {};
    template <> struct storage_representation_impl<h5::reference_t>
        : std::integral_constant<storage_representation_t,
                                 storage_representation_t::scalar> {};
}

/* template specialization from hid_t< .. > type which provides syntactic sugar in the form
 * h5::dt_t<int> dt; 
 * */
namespace h5::impl::detail {
	template<class T> // parent type, data_type is inherited from, see H5Iall.hpp top section for details 
	using dt_p = hid_t<T,H5Tclose,true,true,hdf5::any>;
	/*type id*/
	template<class T>
		struct hid_t<T,H5Tclose,true,true,hdf5::type> : public dt_p<T> {
		using parent = dt_p<T>;
		using hidtype = T;
		hid_t( std::initializer_list<::hid_t> fd ) : parent( fd ){}
		hid_t() : parent( H5I_UNINIT){}
	};
	template <class T> using dt_t = hid_t<T,H5Tclose,true,true,hdf5::type>;
	//
	template <> struct hid_t<h5::reference_t, H5Tclose,true,true, hdf5::type> : public dt_p<h5::reference_t> {
		using parent = dt_p<h5::reference_t>;
		using dt_p<h5::reference_t>::hid_t;
		using hidtype = h5::reference_t;

#if H5_VERSION_GE(1,12,0)
		hid_t() : parent( H5Tcopy(H5T_STD_REF) ) {
#else
		hid_t() : parent( H5Tcopy(H5T_STD_REF_DSETREG) ) {
#endif
			}
		};
	}

/* template specialization is for the preceding class, and should be used only for HDF5 ELEMENT types
 * which are in C/C++ the integral types of: char,short,int,long, ... and C POD types. 
 * anything else, the ones which are considered objects/classes are broken down into integral types + container 
 * then pointer read|write is obtained to the continuous slab and delegated to h5::read | h5::write.
 * IF the data is not in a continuous memory region then it must be copied! 
 */

#define H5CPP_REGISTER_TYPE_( C_TYPE, H5_TYPE )                                           \
namespace h5::impl::detail { 	                                                          \
	template <> struct hid_t<C_TYPE,H5Tclose,true,true,hdf5::type> : public dt_p<C_TYPE> {\
		using parent = dt_p<C_TYPE>;                                                      \
		using dt_p<C_TYPE>::hid_t;                                                        \
		using hidtype = C_TYPE;                                                           \
		hid_t() : parent( H5Tcopy( H5_TYPE ) ) { 										  \
			hid_t id = static_cast<hid_t>( *this );                                       \
			if constexpr ( std::is_pointer_v<C_TYPE> )                                    \
					H5Tset_size (id,H5T_VARIABLE), H5Tset_cset(id, H5T_CSET_UTF8);        \
		}                                                                                 \
	};                                                                                    \
}                                                                                         \
namespace h5 {                                                                            \
	template <> struct name<C_TYPE> {                                                     \
		static constexpr char const * value = #C_TYPE;                                    \
	};                                                                                    \
}                                                                                         \
namespace h5::meta {                                                                      \
	/* W4 (review item A7): every type registered through this macro — including all  \
	 * H5CPP_REGISTER_STRUCT(Foo) expansions — marks itself as a known compound, which  \
	 * lets the aggregate storage_representation_impl fallback accept it. Unregistered    \
	 * aggregates remain storage=unsupported and are caught at compile time. */           \
	template <> struct has_registered_compound<C_TYPE> : std::true_type {};               \
}                                                                                         \

/* registering integral data-types for NATIVE ones, which means all data is stored in the same way 
 * in file and memory: TODO: allow different types for file storage
 * */
	H5CPP_REGISTER_TYPE_(bool,H5T_NATIVE_HBOOL)

	H5CPP_REGISTER_TYPE_(unsigned char, H5T_NATIVE_UCHAR) 			H5CPP_REGISTER_TYPE_(char, H5T_NATIVE_CHAR)
	H5CPP_REGISTER_TYPE_(unsigned short, H5T_NATIVE_USHORT) 		H5CPP_REGISTER_TYPE_(short, H5T_NATIVE_SHORT)
	H5CPP_REGISTER_TYPE_(unsigned int, H5T_NATIVE_UINT) 			H5CPP_REGISTER_TYPE_(int, H5T_NATIVE_INT)
	H5CPP_REGISTER_TYPE_(unsigned long int, H5T_NATIVE_ULONG) 		H5CPP_REGISTER_TYPE_(long int, H5T_NATIVE_LONG)
	H5CPP_REGISTER_TYPE_(unsigned long long int, H5T_NATIVE_ULLONG) H5CPP_REGISTER_TYPE_(long long int, H5T_NATIVE_LLONG)
	H5CPP_REGISTER_TYPE_(float, H5T_NATIVE_FLOAT) 					H5CPP_REGISTER_TYPE_(double, H5T_NATIVE_DOUBLE)
	H5CPP_REGISTER_TYPE_(long double,H5T_NATIVE_LDOUBLE)

	H5CPP_REGISTER_TYPE_(char*, H5T_C_S1)
	H5CPP_REGISTER_TYPE_(const char*, H5T_C_S1)

	// dt_t<std::string> = H5T_C_S1 with H5T_VARIABLE size. Without this spec the
	// dt_t primary template returns H5I_UNINIT for std::string fields, causing
	// H5Tinsert("...", offset, UNINIT) to fail (and the dt_t destructor calls
	// H5Tclose(UNINIT) which corrupts the allocator). Surfaces when a struct or
	// std::tuple<...,std::string,...> field is built via the compound dispatch.
	namespace h5::impl::detail {
		// Note: the H5CPP_REGISTER_TYPE_ macro uses `using dt_p<C>::hid_t;` to
		// inherit the parent's constructors. The h5cpp-compiler's clang AST
		// scanner (Clang 10 backed) parses that inheriting-ctor declaration
		// fine on non-templated specialisations but rejects it inside this
		// templated partial specialisation with "member 'hid_t' has the same
		// name as its class". Write the forwarding ctors explicitly to keep
		// both the compiler-tool scan and the regular C++ build happy.
		template <class CharT, class Tr, class A>
		struct hid_t<std::basic_string<CharT, Tr, A>, H5Tclose, true, true, hdf5::type>
			: public dt_p<std::basic_string<CharT, Tr, A>> {
			using parent  = dt_p<std::basic_string<CharT, Tr, A>>;
			using hidtype = std::basic_string<CharT, Tr, A>;
			hid_t( std::initializer_list<::hid_t> fd ) : parent( fd ) {}
			hid_t() : parent( H5Tcopy(H5T_C_S1) ) {
				::hid_t id = static_cast<::hid_t>(*this);
				H5Tset_size(id, H5T_VARIABLE);
				H5Tset_cset(id, H5T_CSET_UTF8);
			}
		};
	}
	namespace h5 {
		template <class CharT, class Tr, class A>
		struct name<std::basic_string<CharT, Tr, A>> {
			static constexpr char const* value = "string";
		};
	}
	namespace h5::meta {
		template <class CharT, class Tr, class A>
		struct has_registered_compound<std::basic_string<CharT, Tr, A>> : std::true_type {};
	}

// half float support: 
// TODO: factor out in a separate file
#ifdef HALF_HALF_HPP
   namespace h5::impl::detail {
	template <> struct hid_t<half_float::half, H5Tclose,true,true, hdf5::type> : public dt_p<half_float::half> {
		using parent = dt_p<half_float::half>;
		using dt_p<half_float::half>::hid_t;
		using hidtype = half_float::half;
		hid_t() : parent( H5Tcopy( H5T_NATIVE_FLOAT ) ) {

			H5Tset_fields( handle, 15, 10, 5, 0, 10);
			H5Tset_precision(handle, 16);
			H5Tset_ebias( handle, 15);
			H5Tset_size(handle,2);
			hid_t id = static_cast<hid_t>( *this );
		}
	};
}
namespace h5 {
	template <> struct name<half_float::half> {
		static constexpr char const * value = "half-float";
	};
}
template<> struct h5::meta::is_contiguous<std::vector<half_float::half>> : std::true_type {};
#endif
// Open XDR doesn-t define namespace or 
#ifdef WITH_OPENEXR_HALF 
   namespace h5::impl::detail {
	template <> struct hid_t<OPENEXR_NAMESPACE::half, H5Tclose,true,true, hdf5::type> : public dt_p<OPENEXR_NAMESPACE::half> {
		using parent = dt_p<OPENEXR_NAMESPACE::half>;
		using dt_p<OPENEXR_NAMESPACE::half>::hid_t;
		using hidtype = OPENEXR_NAMESPACE::half;
		hid_t() : parent( H5Tcopy( H5T_NATIVE_FLOAT ) ) {

			H5Tset_fields( handle, 15, 10, 5, 0, 10);
			H5Tset_precision(handle, 16);
			H5Tset_ebias( handle, 15);
			H5Tset_size(handle,2);
			hid_t id = static_cast<hid_t>( *this );
		}
	};
}
namespace h5 {
	template <> struct name<OPENEXR_NAMESPACE::half> {
		static constexpr char const * value = "openexr half-float";
	};
}
template<> struct h5::meta::is_contiguous<std::vector<OPENEXR_NAMESPACE::half>> : std::true_type {};

#endif

#if __cplusplus >= 202302L && defined(__STDCPP_FLOAT16_T__)
#include <stdfloat>
namespace h5::impl::detail {
    template <> struct hid_t<std::float16_t, H5Tclose,true,true, hdf5::type> : public dt_p<std::float16_t> {
        using parent = dt_p<std::float16_t>;
        using dt_p<std::float16_t>::hid_t;
        using hidtype = std::float16_t;
        hid_t() : parent( H5Tcopy( H5T_NATIVE_FLOAT ) ) {
            H5Tset_fields( handle, 15, 10, 5, 0, 10);
            H5Tset_precision(handle, 16);
            H5Tset_ebias( handle, 15);
            H5Tset_size(handle, 2);
        }
    };
}
namespace h5 {
    template <> struct name<std::float16_t> {
        static constexpr char const * value = "float16_t";
    };
}
template<> struct h5::meta::is_contiguous<std::vector<std::float16_t>> : std::true_type {};
// impl::decay and meta::decay: prevent value_type stripping
namespace h5::meta { template <> struct decay<std::float16_t> { using type = std::float16_t; }; }
namespace h5::impl {
    namespace detail { template <> struct has_explicit_decay<std::float16_t> : std::true_type {}; }
    template <> struct decay<std::float16_t> { using type = std::float16_t; };
}
// storage_traits_impl_t: override the arithmetic fallback which returns H5I_INVALID_HID
// for unrecognized types; std::float16_t requires a custom constructed type.
namespace h5::meta {
    template <>
    struct storage_traits_impl_t<std::float16_t> {
        static constexpr bool supported   = true;
        static constexpr bool owns_handle = true;
        static ::hid_t create_type() noexcept {
            ::hid_t tid = H5Tcopy(H5T_NATIVE_FLOAT);
            H5Tset_fields(tid, 15, 10, 5, 0, 10);
            H5Tset_precision(tid, 16);
            H5Tset_ebias(tid, 15);
            H5Tset_size(tid, 2);
            return tid;
        }
    };
}
#endif

// std::complex<T>: H5T_COMPLEX native type (HDF5 >= 2.0) or compound fallback
namespace h5::impl::detail {
#if H5_VERSION_GE(2,0,0)
    template <> struct hid_t<std::complex<float>, H5Tclose,true,true, hdf5::type>
            : public dt_p<std::complex<float>> {
        using parent = dt_p<std::complex<float>>;
        using dt_p<std::complex<float>>::hid_t;
        using hidtype = std::complex<float>;
        hid_t() : parent( H5Tcomplex_create(H5T_NATIVE_FLOAT) ) {}
    };
    template <> struct hid_t<std::complex<double>, H5Tclose,true,true, hdf5::type>
            : public dt_p<std::complex<double>> {
        using parent = dt_p<std::complex<double>>;
        using dt_p<std::complex<double>>::hid_t;
        using hidtype = std::complex<double>;
        hid_t() : parent( H5Tcomplex_create(H5T_NATIVE_DOUBLE) ) {}
    };
    template <> struct hid_t<std::complex<long double>, H5Tclose,true,true, hdf5::type>
            : public dt_p<std::complex<long double>> {
        using parent = dt_p<std::complex<long double>>;
        using dt_p<std::complex<long double>>::hid_t;
        using hidtype = std::complex<long double>;
        hid_t() : parent( H5Tcomplex_create(H5T_NATIVE_LDOUBLE) ) {}
    };
#else
    // HDF5 < 2.0.0: two-field compound matching std::complex<T> contiguous layout
    template <> struct hid_t<std::complex<float>, H5Tclose,true,true, hdf5::type>
            : public dt_p<std::complex<float>> {
        using parent = dt_p<std::complex<float>>;
        using dt_p<std::complex<float>>::hid_t;
        using hidtype = std::complex<float>;
        hid_t() : parent( H5Tcreate(H5T_COMPOUND, sizeof(std::complex<float>)) ) {
            H5Tinsert(handle, "r", 0,             H5T_NATIVE_FLOAT);
            H5Tinsert(handle, "i", sizeof(float), H5T_NATIVE_FLOAT);
        }
    };
    template <> struct hid_t<std::complex<double>, H5Tclose,true,true, hdf5::type>
            : public dt_p<std::complex<double>> {
        using parent = dt_p<std::complex<double>>;
        using dt_p<std::complex<double>>::hid_t;
        using hidtype = std::complex<double>;
        hid_t() : parent( H5Tcreate(H5T_COMPOUND, sizeof(std::complex<double>)) ) {
            H5Tinsert(handle, "r", 0,              H5T_NATIVE_DOUBLE);
            H5Tinsert(handle, "i", sizeof(double), H5T_NATIVE_DOUBLE);
        }
    };
    template <> struct hid_t<std::complex<long double>, H5Tclose,true,true, hdf5::type>
            : public dt_p<std::complex<long double>> {
        using parent = dt_p<std::complex<long double>>;
        using dt_p<std::complex<long double>>::hid_t;
        using hidtype = std::complex<long double>;
        hid_t() : parent( H5Tcreate(H5T_COMPOUND, sizeof(std::complex<long double>)) ) {
            H5Tinsert(handle, "r", 0,                   H5T_NATIVE_LDOUBLE);
            H5Tinsert(handle, "i", sizeof(long double), H5T_NATIVE_LDOUBLE);
        }
    };
#endif
}
namespace h5 {
    template <> struct name<std::complex<float>>
        { static constexpr char const* value = "complex<float>"; };
    template <> struct name<std::complex<double>>
        { static constexpr char const* value = "complex<double>"; };
    template <> struct name<std::complex<long double>>
        { static constexpr char const* value = "complex<long double>"; };
}
template<> struct h5::meta::is_contiguous<std::vector<std::complex<float>>>       : std::true_type {};
template<> struct h5::meta::is_contiguous<std::vector<std::complex<double>>>      : std::true_type {};
template<> struct h5::meta::is_contiguous<std::vector<std::complex<long double>>> : std::true_type {};

// std::complex<T> is standard-layout and contiguous in memory (C++11 §26.4) but not
// trivial, so the generic is_transport_contiguous rule rejects it — supply explicit overrides.
namespace h5::meta {
    template <class T> struct is_transport_contiguous_impl_t<std::complex<T>,       void> : std::true_type {};
    template <class T> struct is_transport_contiguous_impl_t<std::vector<std::complex<T>>, void> : std::true_type {};
}

// std::complex<T>::value_type is T, so both h5::meta::decay and h5::impl::decay resolve
// std::complex<T> to its scalar component T via the generic value_type fallback.  Override
// both to keep the element type intact so the HDF5 compound type descriptor is used for I/O.
namespace h5::meta {
    template <class T> struct decay<std::complex<T>> { using type = std::complex<T>; };
}
namespace h5::impl {
    namespace detail {
        template <class T> struct has_explicit_decay<std::complex<T>> : std::true_type {};
    }
    template <class T> struct decay<std::complex<T>> { using type = std::complex<T>; };
}

// std::pair<K,V> compound HDF5 type — two-field compound with "first" and "second".
// Requires K and V to be standard-layout so offsetof is valid.
namespace h5::impl::detail {
    template <class K, class V>
    struct hid_t<std::pair<K,V>, H5Tclose, true, true, hdf5::type>
        : public dt_p<std::pair<K,V>> {
        using parent = dt_p<std::pair<K,V>>;
        using hidtype = std::pair<K,V>;
        hid_t() : parent( H5Tcreate(H5T_COMPOUND, sizeof(std::pair<K,V>)) ) {
            using pair_t = std::pair<std::remove_cv_t<K>, std::remove_cv_t<V>>;
            dt_t<std::remove_cv_t<K>> dt1;
            dt_t<std::remove_cv_t<V>> dt2;
            H5Tinsert(this->handle, "first",  offsetof(pair_t, first),  static_cast<::hid_t>(dt1));
            H5Tinsert(this->handle, "second", offsetof(pair_t, second), static_cast<::hid_t>(dt2));
        }
    };
}
namespace h5 {
    template <class K, class V> struct name<std::pair<K,V>>
        { static constexpr char const* value = "pair<K,V>"; };
}

// Scalar std::pair<K,V> needs impl::data / impl::size for the by-value-return
// h5::read<pair>(ds) path at H5Dread.hpp:638 — the generic data() guard in
// H5Mstl.hpp requires is_trivial_v which std::pair fails (user ctors). pair
// IS standard-layout, so &ref is a valid pointer into the compound buffer.
namespace h5::impl {
    template <class K, class V>
    inline const std::pair<K,V>* data(const std::pair<K,V>& p) noexcept { return &p; }
    template <class K, class V>
    inline       std::pair<K,V>* data(std::pair<K,V>& p)       noexcept { return &p; }
    template <class K, class V>
    inline std::array<std::size_t, 0> size(const std::pair<K,V>&) noexcept { return {}; }
    template <class K, class V> struct rank<std::pair<K,V>>
        : std::integral_constant<size_t, 0> {};
}

// std::tuple<Ts...> compound HDF5 type — field layout from h5::meta::detail::tuple_layout.
// std::tuple is not standard-layout, so a custom flat-buffer layout is used for I/O.
namespace h5::impl::detail {
    template <class... Ts>
    struct hid_t<std::tuple<Ts...>, H5Tclose, true, true, hdf5::type>
        : public dt_p<std::tuple<Ts...>> {
        using parent = dt_p<std::tuple<Ts...>>;
        using hidtype = std::tuple<Ts...>;
        hid_t() : parent( H5Tcreate(H5T_COMPOUND,
                           h5::meta::detail::tuple_layout<Ts...>::total_size()) ) {
            static constexpr const char* field_names[] = {
                "_0","_1","_2","_3","_4","_5","_6","_7","_8","_9",
                "_10","_11","_12","_13","_14","_15","_16","_17","_18","_19",
                "_20","_21","_22","_23","_24","_25","_26","_27","_28","_29",
                "_30","_31"
            };
            insert_fields(std::make_index_sequence<sizeof...(Ts)>{}, field_names);
        }
    private:
        template <std::size_t... Is>
        void insert_fields(std::index_sequence<Is...>, const char* const* names) {
            (insert_one<Is>(names[Is]), ...);
        }
        template <std::size_t I>
        void insert_one(const char* name) {
            using elem_t = std::tuple_element_t<I, std::tuple<Ts...>>;
            dt_t<std::remove_cv_t<elem_t>> dt;
            H5Tinsert(this->handle, name,
                h5::meta::detail::tuple_layout<Ts...>::template offset<I>(),
                static_cast<::hid_t>(dt));
        }
    };
}
namespace h5 {
    template <class... Ts> struct name<std::tuple<Ts...>>
        { static constexpr char const* value = "tuple<...>"; };
}

// Explicit access_traits_t for std::vector<std::complex<T>> so the I/O dispatch
// selects the contiguous path (c.data() / c.size()) rather than the pointers path.
namespace h5::meta {
    namespace detail {
        template <class T, class A>
        struct has_explicit_access_traits<std::vector<std::complex<T>, A>> : std::true_type {};
    }
    template <class T, class A>
    struct access_traits_t<std::vector<std::complex<T>, A>> {
        using element_t = std::complex<T>;
        using pointer_t = const std::complex<T>*;
        static constexpr access_t kind = access_t::contiguous;
        static constexpr bool is_trivially_packable = true;
        static const std::complex<T>* data(const std::vector<std::complex<T>,A>& c) noexcept { return c.data(); }
        static std::complex<T>*       data(std::vector<std::complex<T>,A>& c)       noexcept { return c.data(); }
        static std::array<std::size_t,1> size(const std::vector<std::complex<T>,A>& c) noexcept { return {c.size()}; }
        static std::size_t bytes(const std::vector<std::complex<T>,A>& c) noexcept { return c.size() * sizeof(std::complex<T>); }
    };
}

#define H5CPP_REGISTER_STRUCT( POD_STRUCT ) H5CPP_REGISTER_TYPE_( POD_STRUCT, h5::register_struct<POD_STRUCT>() )

/**
 * @brief Register a primitive C++ type with the HDF5 datatype system.
 *
 * Wraps the boilerplate that h5cpp needs to recognise a user-defined type as a
 * first-class scalar: the `dt_t<T>` (a.k.a. `hid_t<T, H5Tclose, true, true, hdf5::type>`)
 * specialization that emits the HDF5 type id, and the `h5::name<T>` specialization
 * that gives the type a stable string identity for introspection.
 *
 * @param TYPE         C++ type to register (must be a complete type at the macro site)
 * @param NAME         String literal printed by `h5::name<TYPE>::value`
 * @param CREATE_EXPR  Any expression producing an `hid_t` — typically
 *                     `H5Tcopy(H5T_NATIVE_*)`, `H5Tcreate(H5T_OPAQUE, n)`, or
 *                     `H5Tenum_create(H5T_NATIVE_*)`. The result becomes the
 *                     dataset/attribute type for `TYPE`.
 * @param BODY         Brace-enclosed block that runs after construction with
 *                     `handle` (the produced hid_t) in scope. Use `{}` if no
 *                     post-construction setup is needed. Common operations:
 *                     `H5Tset_precision`, `H5Tset_tag`, `H5Tenum_insert`.
 *
 * Usage examples:
 * @code
 *   // Strong typedef — same memory layout as float, distinct name on disk.
 *   H5CPP_REGISTER_DATATYPE(Celsius, "Celsius",
 *       H5Tcopy(H5T_NATIVE_FLOAT), {})
 *
 *   // N-bit packed integer.
 *   H5CPP_REGISTER_DATATYPE(my::n_bit, "my::n_bit",
 *       H5Tcopy(H5T_NATIVE_UCHAR), {
 *           H5Tset_precision(handle, 2);
 *       })
 *
 *   // Opaque blob.
 *   H5CPP_REGISTER_DATATYPE(my::blob, "my::blob",
 *       H5Tcreate(H5T_OPAQUE, 1), {
 *           H5Tset_tag(handle, "my::blob");
 *       })
 *
 *   // Strong enum mapped to H5T_ENUM with named values.
 *   H5CPP_REGISTER_DATATYPE(Status, "Status",
 *       H5Tenum_create(H5T_NATIVE_UINT8), {
 *           Status v;
 *           v = Status::Active;   H5Tenum_insert(handle, "Active",   &v);
 *           v = Status::Inactive; H5Tenum_insert(handle, "Inactive", &v);
 *       })
 * @endcode
 *
 * Multi-template-argument types (e.g. `std::tuple<int, float>`) must be aliased
 * via `using` first — the C preprocessor sees commas as argument separators.
 */
#define H5CPP_REGISTER_DATATYPE( TYPE, NAME, CREATE_EXPR, BODY )                \
    namespace h5 {                                                              \
        template <> struct name<TYPE> {                                         \
            static constexpr char const* value = NAME;                          \
        };                                                                      \
    }                                                                           \
    namespace h5::impl::detail {                                                \
        template <>                                                             \
        struct hid_t<TYPE, H5Tclose, true, true, hdf5::type>                    \
            : public dt_p<TYPE> {                                               \
            using parent  = dt_p<TYPE>;                                         \
            using dt_p<TYPE>::hid_t;                                            \
            using hidtype = TYPE;                                               \
            hid_t() : parent( CREATE_EXPR ) BODY                                \
        };                                                                      \
    }                                                                           \
    /**/

namespace h5 {
    /**
     * @brief Compile-time trait to determine if a type uses scatter/gather I/O.
     *
     * The h5cpp-compiler emits specializations of h5::scatter<T> and h5::gather<T>
     * for user-defined structs with heap-indirection fields (std::vector, std::string,
     * etc.). The H5CPP_REGISTER_SCATTER(T) macro sets this trait to true_type.
     *
     * @tparam T C++ type being queried
     */
    template <typename T> struct has_scatter : std::false_type {};
}

/**
 * @brief Registers a type as scatter/gather eligible.
 *
 * Place this macro at file scope next to the compiler-generated scatter/gather
 * specializations. It sets h5::has_scatter<T> to std::true_type, enabling
 * dispatch in h5::write and h5::read.
 */
#define H5CPP_REGISTER_SCATTER( SCATTER_TYPE ) \
    template<> struct h5::has_scatter<SCATTER_TYPE> : std::true_type {}

/* type alias is responsible for ALL type maps through H5CPP if you want to screw things up
 * start here.
 * template parameters:
 *  hid_t< C_TYPE being mapped, conversion_from_capi, conversion_to_capi, marker_for_this_type>
 * */


namespace h5 {
	template <class T> using dt_t = h5::impl::detail::hid_t<T,H5Tclose,true,true,h5::impl::detail::hdf5::type>;

	template<class T>
	inline hid_t copy( const h5::dt_t<T>& dt ){
		hid_t id = static_cast<hid_t>(dt);
		H5Iinc_ref( id );
		return id;
	}
}


template<class T>
inline std::ostream& operator<<(std::ostream &os, const h5::dt_t<T>& dt) {
	hid_t id = static_cast<hid_t>( dt );
	os << "data type: " << h5::name<T>::value << " ";
	os << ( std::is_pointer_v<T> ? "pointer" : "value" );
	os << ( H5Iis_valid( id ) > 0 ? " valid" : " invalid");

	size_t spos, epos, esize, mpos, msize;
	switch( H5Tget_class( id ) ){
		case H5T_INTEGER: ; break;
		case H5T_FLOAT:
			H5Tget_fields( id, &spos, &epos, &esize, &mpos, &msize );

			os << "\n\tebias: " << H5Tget_ebias( id ) 
				<< " norm: " <<  H5Tget_norm( id ) 
				<< " offset: " <<  H5Tget_offset( id )
			   	<< " precision: " << H5Tget_precision( id )
			   	<< " size: " << H5Tget_size( id )
				<< "\n\tspos:" << spos << " epos:" << epos << " esize:" << esize << " mpos:" << mpos <<" msize:" << msize
			   	<<"\n";
			 break;
		case H5T_STRING: ; break;
		case H5T_BITFIELD: ; break;
		case H5T_OPAQUE: ; break;
		case H5T_COMPOUND: ; break;
		case H5T_REFERENCE: ; break;
		case H5T_ENUM: ; break;
		case H5T_VLEN: ; break;
		case H5T_ARRAY: ;break;
		default: break;
	}
	/*
*/

	return os;
}

namespace h5::meta {
// Dispatch wrapper: prefers new type engine when supported, falls back to dt_t<T> for
// unregistered compound types registered via H5CPP_REGISTER_STRUCT.
template <class T>
class resolved_type_t {
    ::hid_t id_;
    bool    owns_;
    std::optional<h5::dt_t<T>> fallback_;
public:
    resolved_type_t() noexcept {
        if constexpr (storage_traits_t<T>::supported) {
            id_   = storage_traits_t<T>::create_type();
            owns_ = storage_traits_t<T>::owns_handle;
        } else {
            fallback_.emplace();
            id_   = static_cast<::hid_t>(*fallback_);
            owns_ = false;
        }
    }
    ~resolved_type_t() noexcept {
        if (owns_ && H5Iis_valid(id_)) H5Tclose(id_);
    }
    operator ::hid_t() const noexcept { return id_; }
    // Allows H5Dcreate's custom dt_t override path: type = static_cast<hid_t>(custom_dt)
    resolved_type_t& operator=(::hid_t id) noexcept {
        if (owns_ && H5Iis_valid(id_)) H5Tclose(id_);
        fallback_.reset();
        id_   = id;
        owns_ = H5Iis_valid(id_);
        return *this;
    }
    resolved_type_t(const resolved_type_t&) = delete;
    resolved_type_t& operator=(const resolved_type_t&) = delete;
};
} // namespace h5::meta
