# Copyright (c) 2017-2020 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>
#

# Modify this list to add H5CPP aware libraries, 
linalg = eigen3 armadillo blaze blitz itpp ublas dlib
to_upper = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')

CXXFLAGS += -Werror -I../ -std=gnu++17
CXXFLAGS += $(foreach name,$(linalg), $(shell /usr/bin/pkg-config --silence-errors --cflags $(name)))
CXXFLAGS += $(foreach name,$(linalg), -DH5CPP_USE_$(call to_upper,$(name)))

LIBS += -lgtest -lgtest_main -lhdf5 -lz -ldl -lm  -lpthread -lfmt
LIBS += $(foreach name,$(linalg), $(shell /usr/bin/pkg-config --silence-errors --libs $(name)))

## list test cases here, be certain to begin them with `H`
apps = H5Tmeta H5cmake
#H5E H5Fcreate  H5cmake
all: $(apps)

plots: H5Tmeta
	echo 'compile it with g++-9'
# compile object files 
%.o : $(SRC_DIR)/%.cpp 
	@$(CXX) -o $@ $(CXXFLAGS) -c $^

# link to libraries, and runtest cases as well
H%:  H%.o 
	@$(CXX) $^ $(LIBS) -o $@	
	@./$@

clean_data: 
	@$(RM) *.h5

test: $(apps)

clean:
	@$(RM) *.o *.h5 $(apps) pix/*

.PHONY: test

