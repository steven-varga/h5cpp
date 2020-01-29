#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________
#
apps = arma eigen3 blaze itpp ublas dlib

CXXFLAGS = -Wno-deprecated  -Wall
LIBS =  -lhdf5 -lz -ldl -lm
INCLUDES=/usr/include/eigen3

test: $(apps)
	@./arma
	@./eigen3
	@./itpp
	@./blaze
	@./ublas
	@./dlib

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
	$(CXX) $^ $(LIBS)  -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

dist-clean: clean
.PHONY: test

