#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________
#
# you must include the path to LLVM version specific files  conveniently installed:
# -I/usr/local/h5cpp-llvm
CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include

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
tu-01.h: struct.h
	h5cpp  tu-01.cpp -- $(CXXFLAGS) $(INCLUDES) -Dtu-01.h
tu-02.h: struct.h
	h5cpp  tu-02.cpp -- $(CXXFLAGS) $(INCLUDES)  -Dtu-02.h

# for simplicity let's use explicit rules to compile TU-s into object files
tu-01.o : tu-01.cpp tu-01.h 
	$(CXX) $(INCLUDES) -o tu-01.o  $(CXXFLAGS) -c tu-01.cpp

tu-02.o : tu-02.cpp tu-02.h 
	$(CXX) $(INCLUDES) -o tu-02.o  $(CXXFLAGS) -c tu-02.cpp

main.o : main.cpp 
	$(CXX) -o main.o  $(CXXFLAGS) -c main.cpp

app: tu-01.o tu-02.o main.o 
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5  app

dist-clean: clean
	@$(RM) tu-01.h tu-02.h

.PHONY: test

