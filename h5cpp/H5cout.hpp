/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 *
 */

#ifndef  H5CPP_STD_COUT
#define  H5CPP_STD_COUT

std::ostream& operator<< (std::ostream& os, const h5::dxpl_t& dxpl) {
#ifdef H5_HAVE_PARALLEL
	H5D_mpio_actual_io_mode_t io_mode;
	H5Pget_mpio_actual_io_mode( static_cast<hid_t>(dxpl), &io_mode);

		switch( io_mode ){
			case H5D_MPIO_NO_COLLECTIVE:
				os << "No collective I/O was performed. Collective I/O was not requested or collective I/O isn't possible on this dataset.";
				break;
			case H5D_MPIO_CHUNK_INDEPENDENT:
				os << "HDF5 performed one the chunk collective optimization schemes and each chunk was accessed independently.";
				break;
			case H5D_MPIO_CHUNK_COLLECTIVE:
				os << "HDF5 performed one the chunk collective optimization schemes and each chunk was accessed collectively";
				break;
			case H5D_MPIO_CHUNK_MIXED:
				os <<"HDF5 performed one the chunk collective optimization schemes and some chunks were accessed independently, some collectively";
				break;
			case H5D_MPIO_CONTIGUOUS_COLLECTIVE:
				os <<"Collective I/O was performed on a contiguous dataset.";
			   	break;
		}
#endif
    return os;
}


#endif

