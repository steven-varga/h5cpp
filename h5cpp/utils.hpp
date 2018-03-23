/*
 * Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 * Author: Varga, Steven <steven@vargaconsulting.ca>
 */

#ifndef  H5CPP_UTILS_H 
#define H5CPP_UTILS_H

#include <string>
#include <vector>
#include <stdexcept>
#include <hdf5.h>
#include <string.h>

namespace h5{ namespace utils {
	/**
	 * HDF5 callback implementation for group iterator; 
	 * @param g_id - input
	 * @param name - input
	 * @param info - input
	 * @param op_data - output: vector of strings
	 * @return no error set
	 */
	inline static herr_t h5_callback( hid_t g_id, const char *name, 
			const H5L_info_t *info, void *op_data){
		
		std::vector<std::string> *data =  static_cast<std::vector<std::string>* >(op_data); 
		data->push_back( std::string(name) );
		return 0;
	}

    /**
     * splits full path into path and dataset name ie: /root/data.mat /root - data.mat
     * @param path - full path
     * @return pair of strings containing path and dataset name
     */
    inline std::pair<std::string,std::string> split_path( const std::string& path ){
		int i = path.size(),  k=path.size();
		const char* c = path.data();
		std::string path_, dataset_;
		while ( i && path[i] != '/') i--;

		if( i==0 ){ // only dataset is given
			dataset_ = path[i]!='/' ? path : std::string(c+1,c+k);
			path_ = "/";
		}else{
			dataset_ = std::string(c+i+1,c+k);
			path_ = *c != '/' ?  "/"+ std::string(c,c+i) : std::string(c,c+i);
		}

        return std::pair<std::string,std::string>(path_, dataset_);
	}

    /**
     * 
     * @param fd - file descriptor
     * @param path - full path ie: /solarsystem/earth/canada/toronto.txt
     * @param create - if true the groups are created along the way
     * @return h5 group descriptor -1 in if failure
     */
    inline hid_t group_exist(hid_t fd, const std::string& path, bool create ){

        char *full_path = strdup(path.c_str());
        std::string _path;

        for(char * current=strtok(full_path,"/"); current!=NULL; current=strtok(NULL,"/")){
            _path+= std::string("/")+std::string(current); // extend path

            if( !H5Lexists(fd,_path.c_str(), 0 ) ){
                if( create ) {
                    H5Gclose( H5Gcreate1(fd,_path.c_str(),0) );
                } else {
                    free(full_path);
                    return -1;
                }
			}
        }

        return H5Gopen1(fd,path.c_str());
    }

    /**
     * verifies if dataset exists 
     * @param fd - file descriptor
     * @param path - full path to object
     * @return true if dataset exists otherwise false
     */
    inline bool dataset_exist(hid_t fd, const std::string& path ){

        char *full_path = strdup(path.c_str());
        std::string _path;

        for(char * current=strtok(full_path,"/"); current!=NULL; current=strtok(NULL,"/")){
            _path+= std::string("/")+std::string(current); // extend path

            if( !H5Lexists(fd,_path.c_str(), 0 ) )
                return false;
        }

        return true;
    }
    /**
     * opens group id if exists otherwise returns with error
     * @param fd - file descriptor
     * @param path - full path of hdf5 groups
     * @return - oppen group descriptor or -1 if failure
     */
    inline hid_t opengrp(hid_t fd,  const std::string& path ){
        return group_exist( fd, path, false );
    }

    /**
     * list given directory content
     * @param fd - file descriptor
     * @param directory - path to h5 group
     * @return list of strings 
     */
    inline std::vector<std::string> list_directory(hid_t fd,  const std::string& directory ){
        hid_t group_id = opengrp( fd, directory );
        std::vector<std::string> files;
            H5Literate( group_id, H5_INDEX_NAME, H5_ITER_INC, 0, &h5_callback, &files );
        return files;
    }

    /**
     * returns a pair of vectors containig the current and maximum dimensions of dataset
     * @param fd
     * @param path
     * @return 
     */
    inline std::pair< std::vector<hsize_t>,std::vector<hsize_t> > dimensions( hid_t ds ){

        std::vector<hsize_t> current_dims, max_dims;

        hid_t space = H5Dget_space( ds ); 
        int rank = H5Sget_simple_extent_ndims( space );
        // make  room for return values
        current_dims.resize(rank);max_dims.resize(rank);
        // read the dimensions 
        H5Sget_simple_extent_dims( space, current_dims.data(), max_dims.data() );
        // clean up
        H5Sclose( space );
        return std::pair< std::vector<hsize_t>,std::vector<hsize_t> >
                                                        (current_dims,max_dims);
    }

    /**
     * returns a pair of vectors containig the current and maximum dimensions of dataset
     * @param fd
     * @param path
     * @return 
     */
    inline std::pair< std::vector<hsize_t>,std::vector<hsize_t> > 
                                dimensions(hid_t fd,  const std::string& path ){

        std::vector<hsize_t> current_dims, max_dims;

        if( !dataset_exist(fd,path) ) throw std::runtime_error("dataset doesn't exists: " + path);

        hid_t ds = H5Dopen1( fd, path.data() );
            auto dims = dimensions( ds );
        H5Dclose(ds);
        return dims;
    }

    /**
     * returns a vector describing current dimensions of dataset
     * @param fd
     * @param path
     * @return vector of dimensions with size rank
     */
    inline std::vector<hsize_t> current_dims(hid_t fd,  const std::string& path ){

        auto dims = dimensions(fd,path);
        return dims.first;
    }

    /**
     * returns a vector describing current dimensions of dataset
     * @param ds - dataset descriptor
     * @return vector of dimensions with size rank
     */
    inline std::vector<hsize_t> current_dims(hid_t ds ){

        auto dims = dimensions(ds);
        return dims.first;
    }

    /**
     * returns a vector describing maximum dimensions of dataset, if not extendable
     * dataset then is same as current dimensions
     * @param ds - dataset descriptor
     * @return vector of dimensions with size rank
     */
    inline std::vector<hsize_t> max_dims(hid_t ds ){

        auto dims = dimensions(ds);
        return dims.second;
    }
}}
#endif

