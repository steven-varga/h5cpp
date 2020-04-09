#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include -I./

test: half-float
	./half-float

half-float.o : half-float.cpp
	$(CXX) $(INCLUDES) -o half-float.o  $(CXXFLAGS) -c half-float.cpp

half-float: half-float.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 half-float

dist-clean: clean
.PHONY: test

