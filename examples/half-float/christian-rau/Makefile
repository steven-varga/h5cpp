#  _____________________________________________________________________________
#  Copyright (c) <2020> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018-20 Toronto, On Canada
#  _____________________________________________________________________________

CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include -I./

test: half-float
	./half-float

generated.h: struct.h
	h5cpp  half-float.cpp -- $(CXXFLAGS) $(INCLUDES) -Dgenerated.h

half-float.o : half-float.cpp generated.h 
	$(CXX) $(INCLUDES) -o half-float.o  $(CXXFLAGS) -c half-float.cpp

half-float: half-float.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 half-float

distclean: clean
	@$(RM) generated.h
.PHONY: test

