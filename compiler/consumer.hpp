/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_CONSUMER_HPP 
#define  H5CPP_CONSUMER_HPP

#include <queue>
#include <set>
#include "utils.hpp"

template <typename Producer> class H5TemplateCallback : public MatchFinder::MatchCallback {
public :

	H5TemplateCallback(const std::string& path );
	~H5TemplateCallback();

	virtual void run(const MatchFinder::MatchResult &Result);

private:
	void topological_order( const CXXRecordDecl* node );
	void topological_order( const clang::QualType& qt );

	std::ofstream io;
	Producer producer;
	std::set<const void*> unique, nodes;
	std::queue<std::pair<utils::type,const void*>> store;
};

/* IMPL details */

/** /brief constructs an h5::read<T> | h5::write<T> | h5::create<T> consumer
 * clang AST pattern matcher produces events which are consumed, then tranfers 
 * control to various code generator back-ends.
 * @param path is the output file for generated code   
 */
template <typename Producer>
H5TemplateCallback<Producer>::H5TemplateCallback(const std::string& path ){
	io.open( path );
	producer.file_begin();
}

/** \brief clean up resources 
 */
template <typename Producer>
H5TemplateCallback<Producer>::~H5TemplateCallback(){

	producer.file_end();
	io << producer;
	io.close();
}

/** \brief actual clang callback to capture cxxRecordDecl events
 * @param Result 
 */
template <typename Producer>
void H5TemplateCallback<Producer>::run(const MatchFinder::MatchResult &Result) {
	const CXXRecordDecl* node = Result.Nodes.getNodeAs<clang::CXXRecordDecl>("cxxRecordDecl");
	auto didnt_try = nodes.insert( node );
	// save this, we'll need it to map typedef-s to builtin types
	if( !node || !node->isPOD() || !didnt_try.second  ) return;

	topological_order( node );

	std::string var, type, rn;
	rn = utils::type_name(node);
	producer.template_decl( rn );

	//traverse nodes in topological order
	while( !store.empty() ){
		auto node = store.front();

		const ConstantArrayType* ar;
		const CXXRecordDecl* re;

		switch( node.first ){
			case utils::type::array:
				ar = utils::as<const ConstantArrayType*>( node.second );
				type = producer.cache( utils::element_type_name( ar ) );
				var  = producer.array_decl( type, utils::size( ar ));
				producer.cache_add( utils::type_name( ar ), var );
			break;
			case utils::type::record: // record
				re = utils::as<const CXXRecordDecl*>( node.second );
				var = producer.record_decl(  utils::type_name( re ) );
				for(clang::FieldDecl* fld: re->fields() ){
					type = producer.cache( utils::type_name( fld ) );
					producer.type_insert(var, utils::name( fld ), utils::name( re ), type );
				}
				producer.cache_add( utils::type_name( re ), var );
			break;
			default:
				break;
		}
		store.pop();
	}
	producer.type_release();
	// and exit with footer
	producer.return_type( var );
	unique.clear();
}

/** \brief topological order of types
 */ 
template <typename Producer>
void H5TemplateCallback<Producer>::topological_order( const clang::QualType& qt ){

	std::pair<
		std::set<const void*>::const_iterator, bool> it;
	const ConstantArrayType* ar;

	switch( utils::as<utils::type>( qt ) ){
		case utils::type::array:
			ar = utils::as<const ConstantArrayType*>(qt);
			topological_order( ar->getElementType() );
			it = unique.insert( ar );
			if( it.second ) //has not been visited yet
			   	store.push({utils::type::array, ar});
			break;

		case utils::type::record:
			topological_order( utils::as<const CXXRecordDecl*>( qt ) );
			break;
		default:
			;
	}
}

/** \brief topological ordering of types
 */
template <typename Producer>
void H5TemplateCallback<Producer>::topological_order( const CXXRecordDecl* node ){

	for(clang::FieldDecl* fld: node->fields() )
		topological_order( utils::as< const clang::QualType>( fld ) );
	auto it = unique.insert( node );
	if( it.second ) //has not been visited yet
		store.push( {utils::type::record, node} );
}
#endif
