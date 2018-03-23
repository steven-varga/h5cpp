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

CXXFLAGS = -Werror -I../ -I../external -std=c++14 -DDEBUG
LIBS = -lprofiler -lglog -lgtest -lgtest_main -lboost_date_time -lhdf5 -lhdf5_hl -lz -ldl -lm  -lpthread
flags =  --gtest_print_time=0 --gtest_color=auto

apps = macros integral struct stl armadillo utils eigen3 ublas itpp blitz blaze dlib
all: $(apps)

%.o : $(SRC_DIR)/%.cpp 
	$(CXX) -o $@  $(CXXFLAGS) -c $^

eigen3.o : eigen3.cpp 
	$(CXX) -I/usr/include/eigen3 $(CXXFLAGS) -c -o eigen3.o eigen3.cpp

preprocessor: preprocessor.cpp 
	$(CXX) -I/usr/include/eigen3 -E -std=c++14 preprocessor.cpp

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
eigen3: eigen3.o
	$(CXX) $^ $(LIBS) -o $@	
ublas: ublas.o
	$(CXX) $^ $(LIBS) -o $@	
itpp: itpp.o
	$(CXX) $^ $(LIBS) -litpp -o $@	
blitz: blitz.o
	$(CXX) $^ $(LIBS)  -o $@	
dlib: dlib.o
	$(CXX) $^ $(LIBS)  -o $@	
blaze: blaze.o
	$(CXX) $^ $(LIBS)  -o $@	

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
test-eigen3: eigen3
	./eigen3 $(flags)
test-ublas: ublas
	./ublas $(flags)
test-itpp: itpp
	./itpp $(flags)
test-blitz: blitz
	./blitz $(flags)
test-dlib: dlib
	./dlib $(flags)
test-blaze: blaze
	./blaze $(flags)
test-preprocessor: preprocessor

test: test-utils test-macros test-integral test-struct test-stl test-armadillo test-ublas test-itpp test-blitz test-blaze 

#test: test-dlib 

clean:
	@$(RM) *.o *.h5 $(apps) preprocessor


.PHONY: test

