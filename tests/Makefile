# Copyright (c) 2017-2020 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>
#

# Modify this list to add H5CPP aware libraries, 
linalg = eigen3 armadillo blaze blitz itpp ublas dlib
compilers = clang++ icpc g++-7 g++-8 g++-9 clang++-6.0 clang++-7 clang++-8 clang++-9 clang++-10 
#pgc++
to_upper = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')

CXXFLAGS += -Werror -I../ -std=c++17
CXXFLAGS += $(foreach name,$(linalg), $(shell /usr/bin/pkg-config --silence-errors --cflags $(name)))
#CXXFLAGS += $(foreach name,$(linalg), -DH5CPP_USE_$(call to_upper,$(name)))

LIBS += -lgtest -lgtest_main -lhdf5 -lz -ldl -lm  -lpthread -lfmt
LIBS += $(foreach name,$(linalg), $(shell /usr/bin/pkg-config --silence-errors --libs $(name)))


.PHONY: test-with-compilers

## list test cases here, be certain to begin them with `H`
apps = H5Dwrite
# H5Dread
# H5Meigen H5Marma H5Dwrite H5Sall H5Tmeta H5Dcreate H5Tall H5Fcreate H5Fopen H5Iall H5cmake
#H5E H5Tmeta 
all: $(apps)

plots: H5plot-integral.cpp
	$(MAKE) -f Makefile plots-job CXX=g++-9

plots-job: H5plot-integral
	#H5plot-typemap H5plots

# compile object files 
%.o : $(SRC_DIR)/%.cpp 
	@$(CXX) -o $@ $(CXXFLAGS) -c $^
# link to libraries, and runtest cases as well
H%:  H%.o 
	@$(CXX) $^ $(LIBS) -o $@	
	@./$@

clean_data: 
	@$(RM) *.h5 output-*

test: $(apps)

clean:
	@find . -type f -executable -delete
	@$(RM) *.o *.h5 $(apps) pix/* 

.PHONY: test

##
# execute all test with a set of compilers
# all output redirected to sed script which removes color coding then into output-$(CXX) file
#|  sed -r "s/[[:cntrl:]]\[[0-9]{1,3}m//g"
test-with-compilers:
	@printf "%-.80s %s\n"  "----------------------------------------------------------------------------------------" "----------"
	@printf "%-.80s %s\n"  "compiler                                                                                " "error code"
	@printf "%-.80s %s\n"  "----------------------------------------------------------------------------------------" "----------"
	@for cxx in $(compilers); \
		do  printf "%-.80s  "  "`$$cxx --version | tr -d '\n' `                                                                   ";\
			CXXFLAGS=-DH5CPP_TEST_NOCOLOR $(MAKE) -f Makefile test-with-compilers-recompile CXX=$$cxx > output-$$cxx 2>&1;\
			if [ $$? -eq 0 ]; then echo "[  OK  ]"; else echo "[ FAIL ]"; fi\
	   	done;
	@printf "%-.80s %s\n"  "----------------------------------------------------------------------------------------" "----------"

test-with-compilers-recompile: clean all
	@echo "CXX COMPILER: " $(CXX) `$(CXX) --version`
	
