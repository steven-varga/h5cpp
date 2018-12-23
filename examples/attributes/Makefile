#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________


apps = attributes
CXXFLAGS =  -std=c++11  -I/usr/local/include -I/usr/local/h5cpp-llvm
LIBS =  -lhdf5  -lz -ldl -lm

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
	h5cpp  attributes.cpp -- $(CXXFLAGS)  -Dgenerated.h

attributes.o : attributes.cpp generated.h 
	$(CXX) -o attributes.o  $(CXXFLAGS) -c attributes.cpp

attributes: attributes.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.prof *.h5 $(apps) generated.h

.PHONY: test

