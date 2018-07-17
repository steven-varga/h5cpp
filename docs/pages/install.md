<!---
 Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>
--->


AWS EC2 Cluster setup for Parallel HDF5 as well as serial HDF5 on Ubuntu 16.04LTS systems {#link_h5cpp_install}
====================================================================================================

H5CPP for now has a strict C++17 requirements, which in time will be dropped to c++14. The easiest way to start is to [obtain a generic ubuntu LTS image](https://cloud-images.ubuntu.com/locator/ec2/) go through the following:

GCC-8  from binary, courtesy of [Jonathon F](https://launchpad.net/~jonathonf)
```bash
sudo add-apt-repository ppa:jonathonf/gcc-8.1 
sudo apt-get update 
sudo apt-get upgrade
sudo apt-get install gcc-8 g++-8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 100 --slave /usr/bin/g++ g++ /usr/bin/g++-8
```
CMAKE 3.11 the most recent version can be cloned from [this github repository](https://github.com/Kitware/CMake)
```bash
git clone https://github.com/Kitware/CMake.git
cd CMake && ./bootstrap && make
sudo make install
```

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
cmake  -DCMAKE_INSTALL_PREFIX=/usr/local CMAKE_BUILD_TYPE=MinSizeRel -DLLVM_TARGETS_TO_BUILD=X86  ../llvm
nohup make -j8&
sudo make install
```

For parellel HDF5 you need a POSIX compliant parallel (duh) filesystem. [OrangeFS](https://s3.amazonaws.com/download.orangefs.org/current/source/orangefs-2.9.7.tar.gz) is a good FSF alternative to commercial solutions. This step is optional, alternative solution is to use single write multiple read SWMR mode where 
on each computing node you have a dedicated MPI process for IO. 
```bash
sudo apt-get install -y gcc flex bison libssl-dev libdb-dev linux-source perl make autoconf linux-headers-`uname -r` zip openssl automake autoconf patch g++ libattr1-dev
./configure --with-kernel=/usr/src/linux-headers-$(uname -r) --prefix=/usr/local --enable-shared
make -j4 && sudo make install
make kmod # to build kernel module
sudo make kmod_install
# load module
sudo insmod /lib/modules/4.4.0-1062-aws/kernel/fs/pvfs2/pvfs2.ko
```

[MPI](openmpi.org) is industry standard for supercomputing and is viable alternative to hadoop on clusters. Be sure to enable grid-engine, and set the C compiler to gcc-5, since it failed with gcc-8 on my install, don't forget to verify SGE grid engine execute `ompi_info | grep gridengine`

```bash
gunzip -c openmpi-3.1.1.tar.gz | tar xf -
cd openmpi-3.1.1
#gcc-8 fails for me July 2018
CC=gcc-5 ./configure --with-sge --with-pvfs2 --prefix=/usr/local
make -j4 && sudo make install
```

SGE can be downloaded from here: TODO add link

LINALG:

```bash 
sudo apt install libboost-math-dev # ublas support
```
The best practice is to install all linear algebra systems from sources, starting with BLAS/LAPACK: 
[INTEL MKL][100] | [AMD CML][101] | [ATLAS][102] | [openBLAS][103] | [NETLIB][104] Then following with your C++ LinearAlgebra/scientific library. Be sure that the optimized BLAS/LAPACK is picked up during configuration. In addition to standard functionality you may be interested in 
[SuperLU][200], [Metis][201], [Pardiso][202], [SuiteSparse, UmfPack, Cholmod][203].

Here is the list of C++ supported Scientific/Linear Algebra libraries:
[armadillo][10] [eigen3][12] [blitz][13] [blaze][16] [dlib][15] [itpp][14] [boost: ublas][11] and [ETL][17] will be added soon. If I left your favourite out or lacking of functionality please shoot me an email.


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

