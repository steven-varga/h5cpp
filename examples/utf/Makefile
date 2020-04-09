#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

apps = utf
CXXFLAGS =  -std=c++17
LIBS =  -lhdf5  -lz -ldl -lm
INCLUDES = -I/usr/local/include -I/usr/include

test: $(apps)
	@./utf

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

utf: utf.o
	$(CXX) $^ $(LIBS) -o $@	

clean:
	@$(RM) *.o *.prof *.h5 $(apps) generated.h

dist-clean: clean
.PHONY: test

