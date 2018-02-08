# Copyright (c) 2017 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this  software  and associated documentation files (the "Software"), to deal in
# the Software  without   restriction, including without limitation the rights to
# use, copy, modify, merge,  publish,  distribute, sublicense, and/or sell copies
# of the Software, and to  permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE  SOFTWARE IS  PROVIDED  "AS IS",  WITHOUT  WARRANTY  OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT  SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY  CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER
# IN  AN  ACTION  OF  CONTRACT, TORT OR  OTHERWISE, ARISING  FROM,  OUT  OF  OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### 
# SEE README to instal GTEST !!!

CXX = h5c++
CXXFLAGS =  -I../ -std=c++14
LIBS = -lprofiler -lglog -lgtest -lgtest_main -lboost_date_time
flags =  --gtest_print_time=0 --gtest_color=auto

apps = macros integral struct stl armadillo utils 
all: $(apps)

%.o : $(SRC_DIR)/%.cpp 
	$(CXX) -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

utils: utils.o
	$(CXX) $^ $(LIBS) -o $@
chrono:  chrono.o 
	$(CXX) $^ $(LIBS) -o $@	
integral:  integral.o 
	$(CXX) $^ $(LIBS) -o $@	
struct: struct.o 
	$(CXX) $^ $(LIBS) -o $@	
armadillo: armadillo.o
	$(CXX) $^ $(LIBS) -o $@	
stl: stl.o
	$(CXX) $^ $(LIBS) -o $@	
macros: macros.o 
	$(CXX) $^ $(LIBS) -o $@	

test-utils: utils
	./utils $(flags)
test-macros: macros
	./macros $(flags)
test-integral: integral
	./integral $(flags)
test-struct: struct
	./struct $(flags)
test-stl: stl
	./stl $(flags)
test-armadillo: armadillo
	./armadillo $(flags)

test: test-utils test-macros test-integral test-struct test-stl test-armadillo 

clean:
	@$(RM) *.o *.h5 $(apps)


.PHONY: test

