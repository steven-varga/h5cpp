<!---

 Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
 Author: Varga, Steven <steven@vargaconsulting.ca>

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this  software  and associated documentation files (the "Software"), to deal in
 the Software  without   restriction, including without limitation the rights to
 use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
 of the Software, and to  permit persons to whom the Software is furnished to do
 so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
 IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
--->


`make all` generates doxygen documention into docs/html and compiles `examples/*.cpp`
In `tests` directory there are instruction on google test suit, similarly you find instructions in 
`h5cpp/profiling`

**to build documentation, examples and profile code install:**
```shell
apt install build-essential libhdf5-serial-dev
apt install google-perftools kcachegrind
apt install doxygen doxygen-gui markdown
apt install libarmadillo-dev libeigen3-dev libblitz0-dev libitpp-dev libdlib-dev libboost-all-dev 
```
in addition to above, download [blaze][106] and copy header files to `/usr/local/include`
ETL is slightly trickier, make sure to clone it recursively, and have either gcc 6.3.0 or clang 3.9 or greater
`git clone --recursive https://github.com/wichtounet/etl.git`
`cd etl; CXX=clang++ make`

compilers:
-----------
[GCC 6.1][gcc] supports -std=c++14
[clang 3.4][clang] 

requirements:
-------------
1. installed serial HDF5 libraries:
	- pre-compiled on ubuntu: `sudo apt install libhdf5-serial-dev hdf5-tools hdf5-helpers hdfview`
	- from source: [HDF5 download][5]
	`./configure --prefix=/usr/local --enable-build-mode=production --enable-shared --enable-static --enable-optimization=high --with-default-api-version=v110 --enable-hl`
	`make -j4` then `sudo make install`

2. C++11 or above capable compiler installed HDF5 libraries: `sudo apt install build-essential g++`
3. set the location of the include library, and c++11 or higher flag: `h5c++  -Iyour/project/../h5cpp -std=c++14` or `CFLAGS += pkg-config --cflags h5cpp`
4. optionally include `[ <armadillo> | <Eigen/Dense> | <blaze/Math.h> | ... ]`  header file before including `<h5cpp/all>`




How to install google-test framework:
-------------------------------------

first install google-test sources: `sudo apt-get install libgtest-dev`
make sure you have cmake: `sudo apt-get install cmake` 
you find the source dir here: `cd /usr/src/gtest` 
inside the source directory run `sudo cmake CMakeLists.txt` 
then `sudo make`
copy  libgtest.a and libgtest_main.a to your /usr/lib folder `sudo cp *.a /usr/lib`

specify link in makefile: `LDLIBS = -lgtest -lgtest_main`

eigen3:
--------
requires to specify the location of eigen3 system directory; which is on my system:
`/usr/include/eigen3`

dependencies for testing ALL TESTCASES:
----------------------------------------
have boost ublas installed some way
sudo apt install libarmadillo-dev libitpp-dev libblitz0-dev libdlib-dev 

FAILS:
------


[gcc]: https://gcc.gnu.org/projects/cxx-status.html#cxx14
[clang]: https://clang.llvm.org/cxx_status.html

