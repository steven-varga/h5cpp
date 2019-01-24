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

apps = struct-perf tickdata-perf stl-perf arma-perf
CXXFLAGS = -I../../ -std=c++17
LIBS =  -lprofiler -lboost_program_options -lhdf5 -lz -ldl -lm

%.o : $(SRC_DIR)/%.cpp 
	$(CXX) -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

stl-perf: stl-perf.o 
	$(CXX) $^ $(LIBS) -o $@	
tickdata-perf: tickdata-perf.o 
	$(CXX) $^ $(LIBS) -o $@	
struct-perf: struct-perf.o 
	$(CXX) $^ $(LIBS) -o $@	
arma-perf: arma-perf.o 
	$(CXX) $^ $(LIBS) -o $@	


test: 

profile: struct-profile tickdata-profile stl-profile


clean:
	@$(RM) *.o *.h5 *.callgrind *.prof $(apps)

struct-profile: struct-perf
	./struct-perf
	google-pprof --callgrind ./struct-perf ./struct-perf.prof >struct-perf.callgrind
	google-pprof --text struct-perf struct-perf.prof
	kcachegrind struct-perf.callgrind &>/dev/null

tickdata-profile: tickdata-perf
	./tickdata-perf
	google-pprof --callgrind ./tickdata-perf ./tickdata-perf.prof >tickdata-perf.callgrind
	google-pprof --text tickdata-perf tickdata-perf.prof
	kcachegrind tickdata-perf.callgrind &>/dev/null

stl-profile: stl-perf
	./stl-perf
	google-pprof --callgrind ./stl-perf ./stl-perf.prof >stl-perf.callgrind
	google-pprof --text stl-perf stl-perf.prof
	kcachegrind stl-perf.callgrind &>/dev/null

arma-profile: arma-perf
	./arma-perf
	google-pprof --callgrind ./arma-perf ./arma-perf.prof >arma-perf.callgrind
	google-pprof --text arma-perf arma-perf.prof
	kcachegrind arma-perf.callgrind &>/dev/null

.PHONY: test

