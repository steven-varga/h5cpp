#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

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

