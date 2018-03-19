#!/bin/sh
HDF5_VERSION=hdf5-1_10_1

set -ex
git clone https://github.com/live-clones/hdf5.git
cd hdf5 && git checkout $HDF5_VERSION && ./configure --prefix=/usr/local && make && sudo make install

