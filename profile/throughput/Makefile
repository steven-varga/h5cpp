# Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>


all: tile 
CXXFLAGS =  -g -mavx -O3 -std=c++11  -I/usr/local/include
LIBS =  -lprofiler -lboost_program_options -lhdf5 -lz -ldl -lm

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

tile: tile.o
	$(CXX) $^ $(LIBS) -o $@
	#./$@ && h5dump -pH example.h5

clean:
	@$(RM) *.o *.h5 tile *.prof cube*

tile-cache: tile
	valgrind --tool=cachegrind --cachegrind-out-file=tile.cache ./tile
	cg_annotate tile.cache >> result
	kcachegrind tile.cache

tile-leak: tile
	./tile 
	valgrind ./tile

tile-profile: tile
	./tile
	google-pprof --callgrind ./tile ./tile.prof >tile.callgrind
	google-pprof --text tile tile.prof
	kcachegrind tile.callgrind &>/dev/null

.PHONY: test

