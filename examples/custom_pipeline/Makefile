#  _____________________________________________________________________________
#  Copyright (c) 2018-2021 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _____________________________________________________________________________

apps = pipeline

CXXFLAGS =  -std=c++17 -Wno-deprecated
LIBS =  -lhdf5  -lz -ldl -lm

test: $(apps)
	@./pipeline
	#h5dump -pH 004.h5

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

pipeline: pipeline.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

dist-clean: clean

.PHONY: test

