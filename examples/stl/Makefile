#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

CXXFLAGS =  -std=c++17  -I/usr/local/include
LIBS =  -lhdf5  -lz -ldl -lm

apps = vector

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

clean:
	@$(RM) *.o *.h5 $(apps)

dist-clean: clean
	@$(RM) generated.h

.PHONY: test

