#  _____________________________________________________________________________
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________


apps = kita
CXX=mpic++
CXXFLAGS =  -std=c++17 -Wno-deprecated
LIBS = -lrestvol -lcurl -lyajl -lhdf5  -lz -ldl -lm

test: $(apps)
	@./kita

%.o : $(SRC_DIR)/%.cpp 
	$(CXX)   -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

kita: kita.o
	$(CXX) $^ $(LIBS) -o $@

clean:
	@$(RM) *.o *.prof *.h5 $(apps)

dist-clean: clean

.PHONY: test

