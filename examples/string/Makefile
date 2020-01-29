#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________

CXXFLAGS =  -std=c++17  -I/usr/include
LIBS =  -lhdf5  -lz -ldl -lm

apps = string

test: string
	./string


%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

string: string.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.h5 $(apps)

dist-clean: clean
.PHONY: test

