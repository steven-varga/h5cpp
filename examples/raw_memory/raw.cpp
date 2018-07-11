
#include <h5cpp/all>
#include <cstddef>
#define values 0,1,2,3,4,5,6,7,8,9

int main(){
	h5::fd_t fd = h5::create("raw.h5",H5F_ACC_TRUNC);

    float  af[] = {values};
    double ad[] = {values};
	double* ptr = static_cast<double*>( calloc(10,sizeof(double)) );

	{ // dataset create: 
		auto ds_0 = h5::create<short>(fd,"/type/short with inline dcpl", 
				h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
				h5::create_path | h5::utf8, // optional lcpl with this default settings**
				h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9}, // optional dcpl
				h5::default_dapl ); // optional dapl
		//** lcpl controls how path (or hdf5 name: links) created, `h5::create_path` makes sure that sub paths are created 
		// h5::default_lcpl and h5::lcpl are predifined conveniently such that h5::defeult_lcpl ::= h5::create_path | h5::utf8

		h5::dcpl_t dcpl = h5::chunk{2,3} | h5::fill_value<short>{42} | h5::fletcher32 | h5::shuffle | h5::nbit | h5::gzip{9};
		// same as above, default values implicit, dcpl explicit
		auto ds_1 = h5::create<short>(fd,"/type/short explicit dcpl", h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED}, dcpl);
		// same as above, default values explicit
		auto ds_2 = h5::create<short>(fd,"/type/short default dcpl lcpl", h5::current_dims{10,20}, h5::max_dims{10,H5S_UNLIMITED},
				h5::default_lcpl, dcpl, h5::default_dapl);
		// if only max_dims specified, the current dims is set to max_dims or zero if the dimension is H5S_UNLIMITED
		// making it suitable storage for packet table 
		auto ds_3 = h5::create<short>(fd,"/type/short max_dims", h5::max_dims{10,H5S_UNLIMITED}, // [10 X 0]  
			  h5::chunk{10,1} | h5::gzip{9} );
		// expandable dataset with compression and chunksize explicitly set
        h5::create<std::string>(fd,"/types/string with chunk and compression", h5::max_dims{H5S_UNLIMITED}, h5::chunk{10} | h5::gzip{9} );
	}

    {   // create + write from 1D memory location into a 2D file space 
		h5::write<double>(fd,"dataset",ad, h5::count{1,10});
    }
    {   // read back to memory location: 2D file space -> 1D mem space from specified offset
		h5::read<double>(fd,"dataset", ptr, h5::count{1,8}, h5::offset{0,2} );

		std::cout<<"valued read: ";
			for(int i=0; i<10; i++) std::cout<< ptr[i] <<" ";
		std::cout<<"\n";

       //if you want to change memory offset, manipulate the passed pointer
		h5::read<double>(fd,"dataset", ptr+4, h5::count{1,3} );
    }
	free( ptr );


}

