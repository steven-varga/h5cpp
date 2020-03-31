# LINUX

### H5CPP Headers
For Debian and Red Hat based systems [there are packages available](http://h5cpp.org/download/), or you can check out the latest version from [GitHUB](https://github.com/steven-varga/h5cpp). The headers have no other dependencies than an C++17 capable compiler. However H5CPP often installed along a linear algebra library [full list is here](#linear-algebra)

### HDF5
Often the simplest is to use the one provided with your computer, however it may be outdated, for best result download and compile [the latest version][207] from The HDFGroup website or use the provided [binary packages][206]. The minimum version requirements for HDF5 CAPI is set to v1.10.2.

### Parallel HDF5

### Compiler
[LLVM 7.0.0 and Clang](http://llvm.org/docs/GettingStarted.html#checkout ) is required to compile and run the `h5cpp` source code transformation tool. Be certain there is 50GB free disk and 16GB memory space on the system you compiling to prevent spurious error messages in the compile or linking phase.  AWS EC2 m3.2xlarge instance has suitable local disk and memory space.
```bash
git clone https://git.llvm.org/git/llvm.git/
cd llvm/tools
git clone https://git.llvm.org/git/clang.git/
# begin-optional
cd ../projects
git clone https://git.llvm.org/git/openmp.git/
git clone https://git.llvm.org/git/libcxx.git/
git clone https://git.llvm.org/git/libcxxabi.git/
# end-optional
cd ../../ && mkdir build && cd build
3.0 (quilt)cmake  -DCMAKE_INSTALL_PREFIX=/usr/local CMAKE_BUILD_TYPE=MinSizeRel -DLLVM_TARGETS_TO_BUILD=X86 -DLLVM_USE_LINKER=gold ../llvm
nohup make -j8&
sudo make install build-essential gcc-8 g++-8
```

###Parallel FS and MPI

[OrangeFS](https://s3.amazonaws.com/download.orangefs.org/current/source/orangefs-2.9.7.tar.gz) is a good FSF alternative to commercial solutions, and is a requirement for Parallel HDF5.

```bash
sudo apt-get install -y gcc flex bison libssl-dev libdb-dev linux-source perl make autoconf linux-headers-`uname -r` zip openssl automake autoconf patch g++ libattr1-dev
./configure --with-kernel=/usr/src/linux-headers-$(uname -r) --prefix=/usr/local --enable-shared
make -j4 && sudo make install
make kmod # to build kernel module
sudo make kmod_install
# load module
sudo insmod /lib/modules/4.4.0-1062-aws/kernel/fs/pvfs2/pvfs2.ko
```

[MPI](openmpi.org) is industry standard for HPC clusters and super computers, current version of [OpenMPI][204] **4.0.1** is confirmed to work with HDF5 1.10.5 series and s a building block of [H5CLUSTER][205] reference platform.
```bash
./autogen.pl && ./configure --prefix=/usr/local --with-slurm --with-pmix=/usr/local --enable-mpi1-compatibility --with-libevent=/usr/local --with-hwloc=/usr/local 
--with-ompi-pmix-rte
```


###Linear Algebra

The best practice is to install all linear algebra systems from sources, starting with BLAS/LAPACK: 
[INTEL MKL][100] | [AMD CML][101] | [ATLAS][102] | [openBLAS][103] | [NETLIB][104] Then following with your C++ LinearAlgebra/scientific library. Be sure that the optimized BLAS/LAPACK is picked up during configuration. In addition to standard functionality you may be interested in 
[SuperLU][200], [Metis][201], [Pardiso][202], [SuiteSparse, UmfPack, Cholmod][203].

Here is the list of C++ supported Scientific/Linear Algebra libraries:
[armadillo][10] [eigen3][12] [blitz][13] [blaze][16] [dlib][15] [itpp][14] [boost: ublas][11].  [ETL][17] will be added soon.

### Intel MKL
download intel MKL from their website, and follow instructions, don't forget to set the environment variables by adding `source /opt/intel/bin/compilervars.sh intel64` to `.bashrc`.


# WINDOWS

# MACINTOSH



[10]: http://arma.sourceforge.net/
[11]: http://www.boost.org/doc/libs/1_66_0/libs/numeric/ublas/doc/index.html
[12]: http://eigen.tuxfamily.org/index.php?title=Main_Page#Documentation
[13]: https://sourceforge.net/projects/blitz/
[14]: https://sourceforge.net/projects/itpp/
[15]: http://dlib.net/linear_algebra.html
[16]: https://bitbucket.org/blaze-lib/blaze
[17]: https://github.com/wichtounet/etl

[100]: https://software.intel.com/en-us/mkl
[101]: https://en.wikipedia.org/wiki/AMD_Core_Math_Library
[102]: http://math-atlas.sourceforge.net/
[103]: https://www.openblas.net/
[104]: http://www.netlib.org/blas/

[200]: http://crd-legacy.lbl.gov/~xiaoye/SuperLU/
[201]: http://glaros.dtc.umn.edu/gkhome/metis/metis/overview
[202]: https://www.pardiso-project.org/
[203]: http://faculty.cse.tamu.edu/davis/suitesparse.html
[204]: https://www.open-mpi.org/software/ompi/v4.0/
[205]: http://cluster.vargaconsulting.ca/
[206]: https://www.hdfgroup.org/downloads/hdf5/
[207]: https://www.hdfgroup.org/downloads/hdf5/source-code/

