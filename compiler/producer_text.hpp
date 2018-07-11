/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


#ifndef  H5CPP_PRODUCER_TEXT_HPP 
#define  H5CPP_PRODUCER_TEXT_HPP



struct TextProducer : Producer<TextProducer> {

	void file_begin_impl(){
		io <<indent<< "<file begin>\n";
	};
	void file_end_impl(){
		io <<indent<< "<file end>\n";
	};
	void template_decl_impl(const std::string& name){
		io<<indent<<"<template decl: " << name << ">\n";
	};
	void record_decl_impl(const std::string&var, const std::string& name){
		io << indent << var << " = <record decl: " << name <<">\n";
	};
	void return_type_impl( const std::string& type ){
		io <<indent<< "<return type: " << type << ">\n";
	};
	void array_decl_impl(const std::string&var,  const std::string& type, uint64_t size ){
		io <<indent<< var << " = <array decl: " << type << " " << size <<">\n";
	};
	void type_insert_impl(const std::string& var,  const std::string& field_name,const std::string& record_name, const std::string& type ){
		io <<indent<< "<type insert: " << var << ", " << field_name << ", " << record_name <<", "<<type << ">\n";
	};
	void type_release_impl( const std::string& type ){
		io <<indent<< "<type release: " << type << ">\n";
	};
}
#endif



