#include <string>
#include <vector>
#include <armadillo>
#include <h5cpp/core>


template <class T> void read_test( size_t size ){


}

template <class T> void harness( size_t size ){

	std::cout<<"test case" <<"\t"<<"type"<<"\t"<<"size"<<"\t"<<"time"<<std::endl;
};

template<> void harness<std::vector<std::string>>(size_t size ){

}



