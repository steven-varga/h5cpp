#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________

CFLAGS =   -I/usr/include
CXXFLAGS =  -std=c++14  -I/usr/include
LIBS = -lhdf5  -lz -ldl -lm 

test: before after
	./before
	./after

generated.h: compound.h
	h5cpp  compound.cpp -- $(CXXFLAGS) -I/usr/include/h5cpp-llvm -Dgenerated.h

before.o : compound.c
	$(CC) -o before.o  $(CFLAGS) -c compound.c

before: before.o
	$(CC) $^ $(LIBS) -o $@	

after.o : compound.cpp generated.h 
	$(CXX) -o after.o  $(CXXFLAGS) -c compound.cpp

after: after.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 $(apps) generated.h before after

.PHONY: test
