#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________
CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include

test: struct
	./struct

# compiler assisted introspection will scan TU translation unit, which is the 
# collection of files you're compiling into an object, generates desired output
# from topologically ordered dependency graph. 
# currently POD structs, arrays and integral types are supported, in arbitrary deep
# embedding
#
# with -Dmyfile.h specifies the generated output, which must be sandwiched between
# <h5cpp/core> and <h5cpp/io>
generated.h: struct.h
	h5cpp  struct.cpp -- $(CXXFLAGS) $(INCLUDES) -Dgenerated.h

struct.o : struct.cpp generated.h 
	$(CXX) $(INCLUDES) -o struct.o  $(CXXFLAGS) -c struct.cpp

struct: struct.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 $(apps)  struct

dist-clean: clean
	@$(RM) generated.h
.PHONY: test

