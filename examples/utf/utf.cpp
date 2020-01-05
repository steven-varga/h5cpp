#include <armadillo>
#include <cstdint>
#include <h5cpp/all>
#include <cstddef>


void test_case(const h5::fd_t fd, const std::string& path ){
	arma::mat M = arma::ones(5,6);
	h5::ds_t ds = h5::write(fd, path, M);
	ds[path.data()] = path ;
}

int main(){
	{
		h5::fd_t fd = h5::create("こんにちは世界.h5", H5F_ACC_TRUNC, h5::default_fcpl,
							h5::libver_bounds({H5F_LIBVER_V18, H5F_LIBVER_V18}) );
		std::string utf8 [] = {
			"hello world", "مرحبا بالعالم", "Բարեւ աշխարհ", "Здравей свят","Прывітанне Сусвет","မင်္ဂလာပါကမ္ဘာလောက","你好，世界",
			"Γειά σου Κόσμε","હેલ્લો વિશ્વ","Helló Világ","こんにちは世界","안녕 세상","سلام دنیا","העלא וועלט"};
		for( auto name : utf8 )
			test_case(fd, name);
	}
	{
		try {
			h5::fd_t fd = h5::open("こんにちは世界.h5", H5F_ACC_RDWR);
			auto m = h5::read<arma::mat>(fd, "مرحبا بالعالم" );
		} catch( const h5::error::any& err ){
			std::cerr << "ERROR:" << err.what();
		}
	}
}

