#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________

CXXFLAGS =  -std=c++17  -I/usr/local/include
LIBS =  -lhdf5  -lz -ldl -lm

apps = vector array

test: vector
	./vector
# STL container examples uses generated.h from `../compound` example, where h5cpp compiler 
# automatically produces this file, containing the type descriptors. 
#

generated.h: struct.h
	h5cpp  vector.cpp --  $(CXXFLAGS) -Dgenerated.h

vector.o : vector.cpp generated.h 
	$(CXX) $(INCLUDES) -o vector.o  $(CXXFLAGS) -c vector.cpp

vector: vector.o
	$(CXX) $^ $(LIBS) -o $@

array: array.o
	$(CXX) $^ $(LIBS) -o $@


clean:
	@$(RM) *.o *.h5 $(apps)

dist-clean: clean
	@$(RM) generated.h

.PHONY: test

