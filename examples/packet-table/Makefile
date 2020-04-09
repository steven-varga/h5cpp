#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

CXXFLAGS =  -std=c++17  -I/usr/local/include -I/usr/include/eigen3 
LIBS =  -lhdf5 -lhdf5_hl  -lz -ldl -lm -lblas -llapack

test: packettable
	./packettable
	h5dump -pH example.h5

# compiler assisted introspection will scan TU translation unit, which is the 
# collection of files you're compiling into an object, generates desired output
# from topologically ordered dependency graph. 
# currently POD structs, arrays and integral types are supported, in arbitrary deep
# embedding
#
# with -Dmyfile.h specifies the generated output, which must be sandwiched between
# <h5cpp/core> and <h5cpp/io>
generated.h: struct.h
	h5cpp  packettable.cpp -- $(CXXFLAGS)  -Dgenerated.h

packettable.o : packettable.cpp generated.h 
	$(CXX) -I/usr/include/eigen3 -o packettable.o  $(CXXFLAGS) -c packettable.cpp

packettable: packettable.o
	$(CXX) $^ $(LIBS) -o $@	

packet_batches.o : packet_batches.cpp 
	$(CXX) -o packet_batches.o  $(CXXFLAGS) -c packet_batches.cpp

packet_batches: packet_batches.o
	$(CXX) $^ $(LIBS) -larmadillo -o $@	


clean:
	@$(RM) *.o *.h5 $(apps)  packettable packet_batches

dist-clean: clean
	@$(RM) generated.h
.PHONY: test

