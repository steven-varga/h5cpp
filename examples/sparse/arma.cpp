#include <armadillo>
#include <h5cpp/all>
#include <random>


int main(){

	h5::fd_t fd = h5::create("example.h5",H5F_ACC_TRUNC);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dist(40, 45);

	arma::SpMat<int>M(20,40);
	for(int i = 0, j=dist(gen); i<M.n_elem; i += j)
	   	M[i] = j, j = dist(gen);
	//std::cout << M;
	std::cout <<"size: " << M.n_rows << "x" << M.n_cols 
		<<" state: " << M.vec_state // 0 matrix, 1 colvec, 2 rowvec
	   	<<" non zeros: " <<  M.n_nonzero
		<< " fill rate: " <<  (double)M.n_nonzero / (double)M.n_cols *  (double)M.n_rows   <<"\n";

	auto extra_attributes = std::make_tuple(
		"author", "Steven Varga",  "company","vargaconsulting",  "date", "2019-oct-17");


	/* single IO op to write a sparse matrix
	 */
	h5::gr_t gr = h5::write(fd, "sparse-multi-file.plain", M );
	h5::awrite(gr,extra_attributes);

	/* compression and sub setting is supported, although the interpretation is delegated to software writer
	 * generally it is suggested to use single IO calls instead of chunked access
	 * */
	h5::write(fd, "sparse-multi-file.gzip", M, h5::chunk{254} | h5::gzip{9}, h5::offset{1024});
	arma::mat K(4,4);

	auto spmat = h5::read<arma::sp_mat>(gr);
	std::cout<<"is dense: " << h5::exp::linalg::is_dense<decltype(K)>::value <<"\n";
	std::cout<<"is continuous: " << h5::exp::is_contigious<decltype(K)>::value <<"\n";
	std::cout<<"rank: " << h5::exp::rank<int>::value << "\n";
	std::cout<<"rank: " << h5::exp::rank<std::vector<int>>::value << "\n";

	//std::cout << values <<"\n";
}

