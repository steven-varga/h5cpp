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

CXXFLAGS = -Werror -I../ -I../external -std=c++17 -DDEBUG
LIBS = -lprofiler -lglog -lgtest -lgtest_main -lboost_date_time -lhdf5 -lhdf5_hl -lz -ldl -lm  -lpthread
flags =  --gtest_print_time=0 --gtest_color=auto

apps = integral packettable stl armadillo
all: $(apps)

%.o : $(SRC_DIR)/%.cpp 
	$(CXX) -o $@  $(CXXFLAGS) -c $^

preprocessor: preprocessor.cpp 
	$(CXX) -I/usr/include/eigen3 -E -std=c++14 preprocessor.cpp

integral:  integral.o 
	$(CXX) $^ $(LIBS) -o $@
packettable:  packettable.o 
	$(CXX) $^ $(LIBS) -o $@	
armadillo: armadillo.o
	$(CXX) $^ $(LIBS) -o $@	
stl: stl.o
	$(CXX) $^ $(LIBS) -o $@	

test-integral: integral
	./integral $(flags)
test-packettable: packettable
	./packettable $(flags) -o $@

test-armadillo: armadillo
	./armadillo $(flags)
test-stl: stl
	./stl $(flags)

test:  test-integral test-packettable test-stl test-armadillo

#test: test-dlib 

clean:
	@$(RM) *.o *.h5 $(apps) preprocessor


.PHONY: test

