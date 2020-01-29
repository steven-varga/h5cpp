# Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>
#
CXX = /usr/bin/g++-9
CXXFLAGS = -Werror -I../  -std=gnu++2a -DDEBUG
LIBS = -lprofiler -lglog -lgtest -lgtest_main -lhdf5 -lz -ldl -lm  -lpthread -lfmt
flags =  --gtest_print_time=0 --gtest_color=auto

## list test cases here, be certain to begin them with `H`
apps = H5E
all: $(apps)

# compile object files 
%.o : $(SRC_DIR)/%.cpp 
	$(CXX) -o $@  $(CXXFLAGS) -c $^

# link to libraries, and run test cases as well
H%:  H%.o 
	$(CXX) $^ $(LIBS) -o $@	
	- ./$@
	cat pix/typemap.svg
test: $(apps)

clean:
	@$(RM) *.o *.h5 $(apps)


.PHONY: test

