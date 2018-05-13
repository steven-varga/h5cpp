# Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
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

apps = basics raw stl dlib blaze blitz itpp ublas eigen3 arma  arma-partial struct string

CXXFLAGS = -I../ -I../external -std=c++17 -Wno-deprecated -I/usr/include/eigen3
LIBS =  -lprofiler -lglog -lhdf5 -lhdf5_hl -lz -ldl -lm  -lpthread

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

properties: properties.o
	$(CXX) $^ $(LIBS) -o $@
file: file.o
	$(CXX) $^ $(LIBS) -o $@
raw: raw.o 
	$(CXX) $^ $(LIBS) -o $@	
stl: stl.o 
	$(CXX) $^ $(LIBS) -o $@	
eigen3: eigen3.o 
	$(CXX) -w $^ $(LIBS) -o $@	
arma: arma.o 
	$(CXX) $^ $(LIBS) -o $@
ublas: ublas.o 
	$(CXX) $^ $(LIBS) -o $@	
itpp: itpp.o 
	$(CXX) $^ $(LIBS) -litpp -o $@	
blitz: blitz.o 
	$(CXX) $^ $(LIBS) -lblitz -o $@	
blaze: blaze.o 
	$(CXX) $^ $(LIBS)  -o $@	
dlib: dlib.o 
	$(CXX) $^ $(LIBS)  -o $@	
arma-partial: arma-partial.o 
	$(CXX) $^ $(LIBS) -o $@
struct: struct.o 
	$(CXX) $^ $(LIBS) -o $@	
string: string.o 
	$(CXX) $^ $(LIBS) -o $@	

test: $(apps)
	@./raw
	@./stl
	@./arma
	@./arma-partial
	@./eigen3
	@./ublas
	@./itpp
	@./blitz
	@./blaze
	@./struct
	@./string
	h5dump -H -d string.mat string.h5

#	@./dlib


clean:
	@$(RM) *.o *.prof *.h5 $(apps)

struct-profile:
	./struct
	google-pprof --callgrind ./struct ./struct.prof >struct.callgrind
	google-pprof --text struct struct.prof
	kcachegrind struct.callgrind &>/dev/null

.PHONY: test

