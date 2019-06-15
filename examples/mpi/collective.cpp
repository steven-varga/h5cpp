#include <mpi.h>
#include <armadillo>
#include <h5cpp/all>

// armadillo
//./configure -DCMAKE_INSTALL_PREFIX=/usr/local -DDETECT_HDF5=OFF
int main(int argc, char** argv) {
    int size, rank, name_len;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Get_processor_name(processor_name, &name_len);
/*
	std::cout<<"Hello world from processor" processor_name <<" "
		<< rank <<"" << size <<"\n";

	{ // CREATE - WRITE
		arma::vec V; V.ones() * rank;

		h5::fd_t fd = h5::create("collective.h5",H5F_ACC_TRUNC, h5::fapl_mpiio{MPI_COMM_WORLD, info} ); 	// and a file
		h5::ds_t ds = h5::create<short>(fd,"create then write"
				,h5::current_dims{10,size}
				,h5::max_dims{10,size}
				,h5::chunk{10,1} | h5::fill_value<short>{-1} );
		//h5::write( ds,  M, h5::offset{2,2}, h5::stride{1,3}  );
	}
*/
    MPI_Finalize();
}
