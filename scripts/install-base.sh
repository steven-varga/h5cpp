#!/bin/sh
set -ex

# AWS-EC2 ami-749bc50b

sudo apt-get update

sudo apt-get install -y build-essential  google-perftools kcachegrind doxygen doxygen-gui markdown libboost-program-options-dev
sudo add-apt-repository ppa:jonathonf/gcc-8.1
sudo apt-get install gcc-8 g++-8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 100 --slave /usr/bin/g++ g++ /usr/bin/g++-8

#lin-alg 
sudo apt-get install -y libitpp-dev libblitz0-dev  libeigen3-dev libdlib-dev

# be sure to install armadillo and eigen from source 

