#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________
#
apps = arma eigen3 itpp blaze ublas

CXXFLAGS =  -std=c++17 -Wno-deprecated  -Wall
LIBS =  -lhdf5 -lz -ldl -lm
INCLUDES=/usr/include/eigen3

test: $(apps)
	@./arma
	@./eigen3
	@./itpp
	@./blaze
	@./ublas


%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

arma: arma.o
	$(CXX) $^ $(LIBS) -o $@
blaze: blaze.o
	$(CXX) $^ $(LIBS) -o $@
itpp: itpp.o
	$(CXX) $^ $(LIBS) -litpp -o $@
eigen3: eigen3.o
	$(CXX) $^ $(LIBS)  -o $@
ublas: ublas.o
	$(CXX) $^ $(LIBS)  -o $@

## btlitz fails: incompletetype	 2018 july 11
#blitz: blitz.o
#	$(CXX) $^ $(LIBS)  -o $@

## dlib fails on C++17 with /usr/include/dlib/matrix/matrix.h:1608:38: error: ISO C++17 does not allow dynamic exception specifications 
#dlib: dlib.o
#	$(CXX) $^ $(LIBS)  -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

.PHONY: test

