#  _____________________________________________________________________________
#  Copyright (c) <2020> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018-2020 Toronto, On Canada
#  _____________________________________________________________________________

CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm 
INCLUDES = -I/usr/local/include -I/usr/include -I/usr/include/eigen3 -I./
test: two-bit n-bit
	./n-bit

two-bit.o : two-bit.cpp 
	$(CXX) $(INCLUDES) -o two-bit.o  $(CXXFLAGS) -c two-bit.cpp

two-bit: two-bit.o
	$(CXX) $^ $(LIBS) -o $@	

n-bit.o : n-bit.cpp 
	$(CXX) $(INCLUDES) -o n-bit.o  $(CXXFLAGS) -c n-bit.cpp

n-bit: n-bit.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5  n-bit

distclean: clean
.PHONY: test

