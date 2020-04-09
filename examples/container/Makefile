#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________
apps = datasets

CXXFLAGS =  -std=c++17 -Wno-deprecated
LIBS =  -lhdf5  -lz -ldl -lm

test: $(apps)
	@./datasets

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

datasets: datasets.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

dist-clean: clean

.PHONY: test

