#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

apps = transform
CXXFLAGS =  -std=c++17 
LIBS =  -lhdf5 -lz -ldl -lm
test: $(apps)
	@./transform

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

transform: transform.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

dist-clean: clean
.PHONY: test

