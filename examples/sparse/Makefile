#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________
#
apps = arma

CXXFLAGS = -Wno-deprecated  -Wall
LIBS =  -lhdf5 -lz -ldl -lm

test: clean $(apps)
	@./arma

all: $(apps)

arma: arma.cpp
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

.PHONY: test

