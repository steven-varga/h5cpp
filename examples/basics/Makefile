#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________


apps = basics

CXXFLAGS =  -std=c++11 -I/usr/local/include `pkg-config --cflags hdf5`
LIBS =  `pkg-config --libs hdf5`

test: $(apps)
	@./basics
	#h5dump -pH 004.h5

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

basics: basics.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

.PHONY: test

