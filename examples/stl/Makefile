#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________

CXXFLAGS =  -std=c++11  -I/usr/local/include
LIBS =  -lhdf5  -lz -ldl -lm

apps = vector

test: vector
	./vector

# STL container examples uses generated.h from `../compound` example, where h5cpp compiler 
# automatically produces thhis file, containing the type descriptors. 
#


%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

vector: vector.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.h5 $(apps)

.PHONY: test

