/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_PRODUCER_H 
#define  H5CPP_PRODUCER_H

#include <stack>
#include <ostream>
#include <sstream>
#include <tuple>
#include <streambuf>
#include <iomanip>

template <typename Derived>
struct Producer {

	void llvm_license();
	void h5cpp_license();
	void copyrights();
	void file_begin();
	void file_end();

	bool cache_add(const std::string& key, const std::string& type );
	std::string cache( const std::string& type );

	void template_decl(const std::string& name);
	std::string record_decl(const std::string& name);
	void return_type( const std::string& type );
	std::string array_decl( const std::string& type, uint64_t size );
	void type_insert(const std::string& var,  const std::string& field_name,const std::string& record_name, const std::string& type );
	void type_release();
	friend std::ostream& operator<<(std::ostream& os, const Producer<Derived>& producer){
		os << producer.io.str();
		return os;
	}
	void indent_push();
	void indent_pop();

	private:

	Producer(){};
	friend Derived;
	std::stack<std::string> stack;
	std::string indent;
	std::stringstream io;
	uint64_t rec_c,arr_c;
};

/** \ingroup copyrights
* \brief copyright notices to comply with BSD licence advertising cause
* BSD advertising clause stipulates to reproduce BSD license of clang and llvm 
* library.  
*/
template <typename Derived>
void Producer<Derived>::llvm_license(){
}

/** \ingroup copyright
* \brief additional copyright notices 
* As the library may be part of larger projects, list additional copyrights which
* are to be reproduced in a binary form.
*/
template <typename Derived>
void Producer<Derived>::h5cpp_license(){
}

/** \ingroup copyrights 
* \brief Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
* This project is closed source, and under development, in order to proceed
* obtain permission from vargaconsulting. 
*/
template <typename Derived>
void Producer<Derived>::copyrights( ){
	io <<
		"/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada\n"
		" *     Author: Varga, Steven <steven@vargaconsulting.ca>\n"
 		" */\n";
}

template <typename Derived>
void Producer<Derived>::file_begin(){
	static_cast<Derived*>(this)->file_begin_impl();
}

template <typename Derived>
void Producer<Derived>::file_end(){
	static_cast<Derived*>(this)->file_end_impl();
}
template <typename Derived>
void Producer<Derived>::template_decl(const std::string& name){
	rec_c = arr_c = 0; indent="    ";
	static_cast<Derived*>(this)->template_decl_impl( name );
	indent_push();
}
template <typename Derived>
std::string Producer<Derived>::record_decl(const std::string& name){

	std::stringstream sa;
	sa << "ct_"<< std::setw(2) << std::setfill('0') << rec_c++;
	static_cast<Derived*>(this)->record_decl_impl(sa.str(), name );
	return sa.str();
}
template <typename Derived>
void Producer<Derived>::return_type( const std::string& type ){
	indent_pop();
	static_cast<Derived*>(this)->return_type_impl( type );
}

template <typename Derived>
std::string Producer<Derived>::array_decl( const std::string& type, uint64_t size ){
	std::stringstream sa;
	sa <<"at_"<< std::setw(2) << std::setfill('0') << arr_c++;
	static_cast<Derived*>(this)->array_decl_impl( sa.str(), type, size );
	return sa.str();
}

template <typename Derived>
void Producer<Derived>::type_insert(const std::string& var,  const std::string& field_name, const std::string& record_name, const std::string& type ){
	static_cast<Derived*>(this)->type_insert_impl( var, field_name, record_name, type);
}

template <typename Derived>
void Producer<Derived>::type_release(){
	static_cast<Derived*>(this)->type_release_impl();
}

template <typename Derived>
void Producer<Derived>::indent_push(){
	stack.push(indent); indent+="    ";
}

template <typename Derived>
void Producer<Derived>::indent_pop(){
	indent = stack.top();
	stack.pop();
}

template <typename Derived>
bool Producer<Derived>::cache_add(const std::string& key, const std::string& type ){
	return 	static_cast<Derived*>(this)->cache_add_impl(key, type );
}
template <typename Derived>
std::string Producer<Derived>::cache( const std::string& type ){
	return 	static_cast<Derived*>(this)->cache_impl( type );
}

#endif



