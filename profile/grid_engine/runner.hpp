#include <string>
#include <vector>
#include <armadillo>
#include <h5cpp/core>
#include <h5cpp/io>
#include <chrono>

template <class Base>
struct Runner {
	Runner(size_t grid_row, size_t grid_col ) : grid_row(grid_row), grid_col(grid_col){
	}
	void run(){
		static_cast<Base*>(this)->run_impl();
	}

	size_t grid_row, grid_col, data_transfer;
};

template <class T>
struct VectorWriteTest : public Runner<VectorWriteTest<T>>{
	using element_type    = typename h5::utils::base<T>::type;
	VectorWriteTest(size_t grid_row, size_t grid_col )
		: Runner<VectorWriteTest<T>>(grid_row,grid_col),
   		ref(  T( grid_row )) {
		this->data_transfer = sizeof(element_type) * grid_row;
		fd = h5::create("002.h5", H5F_ACC_TRUNC);
	}
	void run_impl(){
		for(int i=0; i<this->grid_col; i++)
		h5::write(fd,"dataset", ref);
	}
	h5::fd_t fd;
	const T ref;
};

template <class T>
struct VectorReadTest : public Runner<VectorReadTest<T>>{
	using element_type    = typename h5::utils::base<T>::type;
	VectorReadTest(size_t grid_row, size_t grid_col )
		: Runner<VectorWriteTest<T>>(grid_row,grid_col),
   		ref(  T( grid_row )) {
		this->data_transfer = sizeof(element_type) * grid_row;
		fd = h5::create("002.h5", H5F_ACC_TRUNC);
		h5::write(fd,"dataset", ref);
	}
	void run_impl(){
		//h5::read<T>(fd, "dataset", ref );
	}

	h5::fd_t fd;
	const T ref;
};


template <class T> void harness( const std::string& name, size_t grid_row, size_t grid_col ){
	T test_case(grid_row, grid_col);

	std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
		test_case.run();
	std::chrono::system_clock::time_point stop = std::chrono::system_clock::now();
	auto running_time = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

	std::cout<< name << "\t" << grid_row <<"\t"<<grid_col<<"\t"<< test_case.data_transfer <<"\t" << running_time <<"\t";
};




