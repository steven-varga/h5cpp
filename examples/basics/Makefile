#  _____________________________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _____________________________________________________________________________

apps = basics

CXXFLAGS = -std=c++17 -Wall
LIBS =  -lhdf5  -lz -ldl -lm

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

dist-clean: clean

.PHONY: test

