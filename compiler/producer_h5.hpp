/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */


#ifndef  H5CPP_PRODUCER_H5_HPP 
#define  H5CPP_PRODUCER_H5_HPP

#include <iomanip>
#include <random>


std::string get_include_guard( size_t N ){
	std::string str;
	static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz"
										"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	std::random_device rd;
	std::default_random_engine rng(rd());
	std::uniform_int_distribution<> dist(0,sizeof(alphabet)/sizeof(*alphabet)-2);

	std::generate_n(std::back_inserter(str), N, [&]() {
								return alphabet[dist(rng)];
							});
	return str;
}


struct H5Producer : Producer<H5Producer> {

	void file_begin_impl();
	void file_end_impl();
	void template_decl_impl(const std::string& name);
	void record_decl_impl(const std::string&var, const std::string& name);
	void return_type_impl(const std::string& type);
	void array_decl_impl(const std::string&var,  const std::string& type, uint64_t size);
	void type_insert_impl(const std::string& var,  const std::string& field_name,const std::string& record_name, const std::string& type);
	void type_release_impl();

	bool cache_add_impl(const std::string& key, const std::string& type );
	std::string cache_impl( const std::string& type );

	private:
	std::map<const std::string, const std::string> cpp2hid;
	std::vector<std::string> type_cache;
	std::string hid_t_record;
	int count;
};

void H5Producer::file_begin_impl(){
	io <<
		"/* Copyright (c) 2018 vargaconsulting, Toronto,ON Canada\n"
		" *     Author: Varga, Steven <steven@vargaconsulting.ca>\n"
 		" */\n";
	std::string include_guard = "H5CPP_GUARD_" + get_include_guard(5);

	io << "#ifndef " << include_guard << std::endl;
	io << "#define " << include_guard << std::endl << std::endl;
}
void H5Producer::file_end_impl(){
	io << "#endif\n"; //close `include guard`
}
void H5Producer::template_decl_impl(const std::string& record){
	io <<
	"H5CPP_REGISTER_STRUCT("<< record <<");\n"
	"namespace h5{ namespace utils {\n"
	<<indent<< "//template specialization of " << record << " to create HDF5 COMPOUND type\n"
	<<indent<< "template<> hid_t h5type<" << record << ">(){\n";

	cpp2hid =  std::map <const std::string, const std::string> {
		{"_Bool",     "H5T_NATIVE_HBOOL"},
  		{"char",      "H5T_NATIVE_CHAR"  },{"unsigned char",      "H5T_NATIVE_UCHAR" },
  		{"short",     "H5T_NATIVE_SHORT" },{"unsigned short",     "H5T_NATIVE_USHORT" },
  		{"int",       "H5T_NATIVE_INT"   },{"unsigned int",       "H5T_NATIVE_UINT" },
  		{"long",      "H5T_NATIVE_LONG"  },{"unsigned long",      "H5T_NATIVE_ULONG" },
  		{"long long", "H5T_NATIVE_LLONG" },{"unsigned long long", "H5T_NATIVE_ULLONG" },
  		{"float",     "H5T_NATIVE_FLOAT" },
		{"double",    "H5T_NATIVE_DOUBLE"},{"long double",        "H5T_NATIVE_LDOUBLE" },
	};
	type_cache.clear();
}

void H5Producer::record_decl_impl(const std::string&var, const std::string& record_name){
	cpp2hid.insert( std::make_pair(var,var));
	io <<"\n"<< indent << "hid_t " << var << " = H5Tcreate(H5T_COMPOUND, sizeof (" << record_name << "));\n";
}

void H5Producer::return_type_impl(const std::string& var ){
	io << "\n";
	io	<<indent<<"    //if not used with h5cpp framework, but as a standalone code generator then\n"
		<<indent<<"    //the returned 'hid_t " << var << "' must be closed: H5Tclose(" << var << ");\n"
		<<indent<<"    return " << var  << ";\n"
		<<indent<<"};\n"
	"}}\n";

}

void H5Producer::array_decl_impl(const std::string&var,  const std::string& type, uint64_t size){

	// note the postfix 'size' variable: ar01_ = [23];  
	io	<<indent<<"hsize_t " << var << "_[] ={" <<  size  << "};    "
		<<indent<<"hid_t "   << var << " = H5Tarray_create(" << type << ",1," << var << "_" << ");\n";
}

void H5Producer::type_insert_impl(const std::string& var,  const std::string& field_name,
		const std::string& record_name, const std::string& type){
	io <<indent<<"H5Tinsert(" << var << ", \"" << field_name << "\",\tHOFFSET(" << record_name << "," << field_name << ")," << type <<");\n";
}

void H5Producer::type_release_impl(){
	if( type_cache.size() <= 1 ) return;

	io <<"\n" <<indent<<"//closing all hid_t allocations to prevent resource leakage\n"
	<< indent;
	for( size_t i=0; i<type_cache.size()-1; i++ ){
		io << "H5Tclose(" << type_cache[i] <<");";
		io << ( (i+1)%5 ? " " : "\n"+indent);
	}
	io << "\n";
}

bool H5Producer::cache_add_impl(const std::string& key, const std::string& type ){
	type_cache.push_back(type);
	cpp2hid.insert( std::make_pair(key,type));
	return 	true;
}
std::string H5Producer::cache_impl( const std::string& type ){
	return  cpp2hid[type];
}
#endif



