 
#include <armadillo>
#include <vector>
#include <h5cpp/all>

int main(){
    h5::fd_t fd = h5::create("ref.h5", H5F_ACC_TRUNC);
	{
        h5::ds_t ds = h5::create<float>(fd,"01",  
            h5::current_dims{10,20}, h5::chunk{2,2} | h5::fill_value<float>{1} );
        
        h5::reference_t ref = h5::reference(fd, "01", h5::offset{2,2}, h5::count{4,4});
        h5::write(fd, "single reference", ref);
        /* you can factor out `count` this way :  h5::count count{2,2};  */ 
        std::vector<h5::reference_t> idx {
            // The HDF5 CAPI reqires fd + dataset name, instead of hid_t to ds: wishy-washy 
            h5::reference(fd, "01", h5::offset{2,2}, h5::count{4,4}),
            h5::reference(fd, "01", h5::offset{4,8}, h5::count{1,1}),
            h5::reference(fd, "01", h5::offset{6,12}, h5::count{3,3}),
            h5::reference(fd, "01", h5::offset{8,16}, h5::count{2,1})
        };
        // datset shape can be controlled with dimensions, in this case is 2x2
        // and is not related to the selected regions!!! 
        // data type is H5R_DATASET_REGION when dataspace is provided, otherwise OBJECT
        h5::write(fd, "index", idx, h5::current_dims{2,2}, h5::max_dims{H5S_UNLIMITED, 2});
    }
    { // we going to update the regions referenced by the set of region-references 
      // stored in "index"
        h5::ds_t ds = h5::open(fd, "index");
        std::vector color(50, 9);
        // this is to read from selection
        for(auto& ref: h5::read<std::vector<h5::reference_t>>(ds))
            h5::exp::write(ds, ref, color.data());
    }

    { // we are reading back data from the regions, now they all must be 'color' value '9'
        h5::ds_t ds = h5::open(fd, "index");
        // this is to read from selection
        for(auto& ref: h5::read<std::vector<h5::reference_t>>(ds)){
            arma::fmat mat = h5::exp::read<arma::fmat>(ds, ref);
            std::cout << mat << "\n";
        }
    }
    { // for verification
        std::cout << h5::read<arma::fmat>(fd, "01") << "\n\n";

    }

}
