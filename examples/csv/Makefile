#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________

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

distclean: clean
	@$(RM) generated.h
.PHONY: test

