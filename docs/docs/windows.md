# VisualStudio 2019
### Prerequisites 
- download and install [Visual Studio 2019][15] community edition or higher
- [get cmake](https://cmake.org/install/), make sure during installation `cmake` is added to PATH variable
- [download and install][11] the latest HDF5 packages for Windows, in the setup wizzard make certain to change the installation directory to `C:\linalg\hdf5`
- [install git client][16]
- optionally obtain [Intel MKL][17] or [OpenBLAS][18] for linear algebra support. This tutorial will be using Intel MKL.
- [python3][12] 


#### MKL and PowerShell
From menu select `Intel Parallel Studio 2020/compiler 19.1 for intel 64 ...`, then from command prompt start windows powershell: `powershell` to inherit all environment variables previous shell.
To verify:

- `nmake /?` should produce valid output
- `$ENV:VS2019INSTALLDIR` example output: `C:\Program Files (x86)\Microsoft Visual Studio\2019\Community`
- `$ENV:MKLROOT` example output: `C:\Program Files (x86)\IntelSWTools\compilers_and_libraries_2020.0.166\windows\mkl`


### OpenBLAS
- [openblas][11] `git clone https://github.com/xianyi/OpenBLAS`


### Compiling Libraries
Most of these libraries are optional, however the example files are depend on them.  The  install procedure follows:
- `cd $HOME\source\repos` create it if it doesn't exist
- `mkdir build; cd build; cmake -DCMAKE_INSTALL_PREFIX="c:/h5cpp/" -G "NMake Makefiles" ../; nmake; nmake install; cd ../../`

- [gtest][14]
`mkdir build; cd build; cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="c:/h5cpp/" -G "NMake Makefiles" ../; nmake; cd ../../`
copy files manually
	```
	cp .\build\lib\* C:\linalg\lib\
	cp .\build\bin\* C:\linalg\bin\
	cp -r -force .\include\gtest C:\linalg\include\ 
	```

#### Linear Algebra systems:
There is more to a properly installed linear algebra system than the listed steps here.

- [armadillo][100] `https://gitlab.com/conradsnicta/armadillo-code.git`
`mkdir build; cd build; cmake -DBUILD_SHARED_LIBS=OFF -DDETECT_HDF5=OFF -DCMAKE_BUILD_TYPE=MinSizeRel -DCMAKE_INSTALL_PREFIX="c:/linalg/" -G "NMake Makefiles" ../; nmake; nmake install; cd ../../`
`mkdir build; cd build; cmake -G "NMake Makefiles" ../; nmake; nmake install`
- [eigen][102] `git clone https://gitlab.com/libeigen/eigen.git`
- [blitz][103] `git clone https://github.com/blitzpp/blitz.git`
- [blaze][106] `git clone https://bitbucket.org/blaze-lib/blaze.git`
- [dlib][105] `git clone https://github.com/davisking/dlib`
- [ublas][101] `git clone https://github.com/boostorg/ublas.git` is header only, copy the `include/boost` to `h5cpp`
- [ETL][107] `` doesn't have cmake yet

'C:\Program Files\HDF_Group\HDF5\
[10]: https://docs.microsoft.com/en-us/cpp/build/building-on-the-command-line?view=vs-2019
[11]: https://www.hdfgroup.org/downloads/hdf5/ 
[12]: https://www.python.org/downloads/windows/
[13]: https://preshing.com/20170511/how-to-build-a-cmake-based-project/
[14]: https://github.com/google/googletest
[15]: https://visualstudio.microsoft.com/downloads/
[16]: https://git-scm.com/download/win
[17]: https://software.intel.com/en-us/mkl
[18]: https://sourceforge.net/projects/openblas/files/

[100]: http://arma.sourceforge.net/
[101]: http://www.boost.org/doc/libs/1_66_0/libs/numeric/ublas/doc/index.html
[102]: http://eigen.tuxfamily.org/index.php?title=Main_Page#Documentation
[103]: https://sourceforge.net/projects/blitz/
[104]: https://sourceforge.net/projects/itpp/
[105]: http://dlib.net/linear_algebra.html
[106]: https://bitbucket.org/blaze-lib/blaze
[107]: https://github.com/wichtounet/etl


