#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________
#
apps = arma eigen3 blaze itpp ublas dlib

CXXFLAGS = -Wno-deprecated  -Wall
LIBS =  -lhdf5 -lz -ldl -lm
INCLUDES=/usr/include/eigen3

test: $(apps)
	$(foreach app, $(apps), ./$(app) )

all: $(apps)

arma: arma.cpp
	$(CXX) $^ $(LIBS) -o $@

blaze: blaze.cpp
	$(CXX) $^ $(LIBS) -o $@

itpp: itpp.cpp
	$(CXX) $^ $(LIBS) -litpp -o $@

eigen3: eigen3.cpp
	$(CXX)  -I/usr/include/eigen3 $^ $(LIBS)  -o $@

ublas: ublas.cpp
	$(CXX) $^ $(LIBS)  -o $@

dlib: dlib.cpp
	$(CXX) $^ $(LIBS) -ldlib  -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

dist-clean: clean
.PHONY: test

