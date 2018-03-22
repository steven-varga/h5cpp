#!/bin/sh
set -ex

sudo apt-get -qq update
sudo apt-get install -y build-essential  google-perftools kcachegrind doxygen doxygen-gui markdown libboost-program-options-dev
sudo apt-get install -y libitpp-dev libblitz0-dev

