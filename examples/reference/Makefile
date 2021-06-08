#  _________________________________________________________
#  Copyright (c) 2018-2021 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include

test: reference
	./reference
	h5dump ref.h5

reference.o : reference.cpp
	$(CXX) $(INCLUDES) -o reference.o  $(CXXFLAGS) -c reference.cpp

reference: reference.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 $(apps)  reference

dist-clean: clean
.PHONY: test


