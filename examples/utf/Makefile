#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________


apps = utf
CXXFLAGS =  -std=c++11  -I/usr/local/include -I/usr/include
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include

test: $(apps)
	@./utf
	h5dump -pH 001.h5

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

# compiler assisted introspection will scan TU translation unit, which is the 
# collection of files you're compiling into an object, generates desired output
# from topologically ordered dependency graph. 
# currently POD structs, arrays and integral types are supported, in arbitrary deep
# embedding
#

utf.o : utf.cpp 
	$(CXX) -o utf.o  $(CXXFLAGS) -c utf.cpp

utf: utf.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

.PHONY: test

