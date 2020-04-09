#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm -lpthread
INCLUDES = -I/usr/local/include -I/usr/include -I./

test: csv2hdf5
	./csv2hdf5

generated.h: csv2hdf5.cpp
	h5cpp csv2hdf5.cpp -- $(CXXFLAGS) $(INCLUDES) -Dgenerated.h

csv2hdf5.o : csv2hdf5.cpp generated.h 
	$(CXX) $(INCLUDES) -o csv2hdf5.o  $(CXXFLAGS) -c csv2hdf5.cpp

csv2hdf5: csv2hdf5.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 csv2hdf5

dist-clean: clean
	@$(RM) generated.h
.PHONY: test

