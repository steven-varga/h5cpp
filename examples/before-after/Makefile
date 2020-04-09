#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

CFLAGS =   -I/usr/include
CXXFLAGS =  -std=c++17
LIBS = -lhdf5  -lz -ldl -lm 
INCLUDES = -I/usr/local/include -I/usr/include

test: before after
	./before
	./after

generated.h: compound.h
	h5cpp  compound.cpp -- $(CXXFLAGS) $(INCLUDES)  -Dgenerated.h

before.o : compound.c
	$(CC) -o before.o  $(CFLAGS) -c compound.c

before: before.o
	$(CC) $^ $(LIBS) -o $@	

after.o : compound.cpp generated.h 
	$(CXX) $(INCLUDES) -o after.o  $(CXXFLAGS) -c compound.cpp

after: after.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.h5 $(apps) before after

dist-clean: clean
	@$(RM) generated.h
.PHONY: test
