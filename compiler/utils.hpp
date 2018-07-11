/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_COMPILER_UTILS 
#define  H5CPP_COMPILER_UTILS

namespace utils {

	inline std::string get_type_name(const QualType& qt );
	enum class type{ builtin, array, record, invalid };

	template <typename T> uint64_t size( const T* ptr );
	template <typename T> std::string type_name( const T* ptr );
	template <typename T> std::string name( const T* ptr );
	template <typename T> std::string element_type_name( const T* ptr );
	template <typename T, typename F> T as( F );

	template <> std::string type_name( const CXXRecordDecl* ptr ){
   		 return ptr->getQualifiedNameAsString();
	}

	template <> std::string type_name( const ConstantArrayType* ptr ){
		return ptr->desugar().getAsString();
	}

	template <>  std::string type_name<>(const clang::FieldDecl* field ){
		const QualType qt = field->getType();
		auto type = get_type_name( qt );
		return type;
	}

	template <>  std::string name<>(const clang::FieldDecl* field ){
		return field->getNameAsString();
	}

	template <>  std::string name<>(const clang::CXXRecordDecl* field ){
		return field->getQualifiedNameAsString();
	}

	template <> uint64_t size( const ConstantArrayType* ptr ){
  		return ptr->getSize().getLimitedValue();
	}

	template <> std::string element_type_name( const ConstantArrayType* ptr ){
		return get_type_name( ptr->getElementType());
   	}
	// conversions
	template <> const ConstantArrayType* as<>( clang::QualType qt ){
		const clang::Type* tp = qt.getTypePtrOrNull();
		return (const ConstantArrayType*) tp;
	}

	template <> CXXRecordDecl const* as<>(  clang::QualType qt ){
		const clang::Type* tp = qt.getTypePtrOrNull();
		return tp->getAsCXXRecordDecl();
	}

	template <> utils::type as<>( clang::QualType qt ){
		const clang::Type* tp = qt.getTypePtrOrNull();
		if( tp->isConstantArrayType() )	return type::array;
		else if( tp->isBuiltinType() ) return type::builtin;
		else if( tp->isRecordType() &&
			as<const CXXRecordDecl*>(qt)->isPOD() ) return type::record;
		else
			return type::invalid;
	}

	template <> const clang::QualType as<>(  clang::FieldDecl* field ){
		return field->getType();
	}
	template <> const ConstantArrayType* as<>(  const void* ptr ){
		return (const ConstantArrayType*) ptr;
	}
	template <> const CXXRecordDecl* as<>(  const void* ptr ){
		return (const CXXRecordDecl*) ptr;
	}

	std::string get_type_name(const QualType& qt ){
		const clang::Type* tp = qt.getTypePtrOrNull();

		if( tp->isBuiltinType() ){
			QualType dqt = qt->getCanonicalTypeInternal();
			return dqt.getAsString();
		} else if( tp->isRecordType() ){
			CXXRecordDecl* node = tp->getAsCXXRecordDecl();
			return type_name( node );
		} else if( tp->isConstantArrayType() )
			return type_name( (const ConstantArrayType*) tp);
		else
			return "<unknown_type>";
	}
}
#endif

