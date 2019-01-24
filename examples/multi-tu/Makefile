#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________


# you must include the path to LLVM version specific files  conveniently installed:
# -I/usr/local/h5cpp-llvm
CXXFLAGS =  -std=c++17  -I/usr/local/include 
LIBS =  -lhdf5  -lz -ldl -lm

test: app
	./app

# compiler assisted introspection will scan TU translation unit, which is the 
# collection of files you're compiling into an object, generates desired output
# from topologically ordered dependency graph. 
# currently POD structs, arrays and integral types are supported, in arbitrary deep
# embedding
#
# with -Dmyfile.h specifies the generated output, which must be sandwiched between
# <h5cpp/core> and <h5cpp/io>

# invoke `h5cpp` compiler separately for any given TU translation unit
# to produce tu_0?.h header file which must be included exactly once in each given TU. 
tu_01.h: struct.h
	h5cpp  tu_01.cpp -- $(CXXFLAGS) -I/usr/include/h5cpp-llvm -Dtu_01.h
tu_02.h: struct.h
	h5cpp  tu_02.cpp -- $(CXXFLAGS) -I/usr/include/h5cpp-llvm -Dtu_02.h

# for simplicity let's use explicit rules to compile TU-s into object files
tu_01.o : tu_01.cpp tu_01.h 
	$(CXX) -o tu_01.o  $(CXXFLAGS) -c tu_01.cpp

tu_02.o : tu_02.cpp tu_02.h 
	$(CXX) -o tu_02.o  $(CXXFLAGS) -c tu_02.cpp

main.o : main.cpp 
	$(CXX) -o main.o  $(CXXFLAGS) -c main.cpp

app: tu_01.o tu_02.o main.o 
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 $(apps) tu_01.h tu_02.h app

.PHONY: test

