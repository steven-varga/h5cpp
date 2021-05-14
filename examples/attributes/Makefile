#  _____________________________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _____________________________________________________________________________

apps = attributes
CXXFLAGS =  -std=c++17 
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include

test: $(apps)
	@./attributes
	#h5dump -pH 001.h5

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

# compiler assisted introspection will scan TU translation unit, which is the 
# collection of files you're compiling into an object, generates desired output
# from topologically ordered dependency graph. 
# currently POD structs, arrays and integral types are supported, in arbitrary deep
# embedding
#
# with -Dmyfile.h specifies the generated output, which must be sandwiched between
# <h5cpp/core> and <h5cpp/io>
generated.h: struct.h
	h5cpp  attributes.cpp --  $(CXXFLAGS) -Dgenerated.h

attributes.o : attributes.cpp generated.h 
	$(CXX) -o attributes.o  $(CXXFLAGS) -c attributes.cpp

attributes: attributes.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

dist-clean: clean
	@$(RM) generated.h
.PHONY: test

