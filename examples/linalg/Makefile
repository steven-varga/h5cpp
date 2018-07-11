#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________
#
apps = arma 

CXXFLAGS =  -std=c++17 -Wno-deprecated 
LIBS =  -lhdf5 -lz -ldl -lm
test: $(apps)
	@./arma

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

arma: arma.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

.PHONY: test

