# Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>

apps = h5cpp

PREFIX = /usr/local
CXXFLAGS =  -I../  `llvm-config --cxxflags` -std=c++17
LDFLAGS = `llvm-config --ldflags`
LIBS =  -lclangFrontend -lclangSerialization -lclangDriver \
		-lclangTooling -lclangParse -lclangSema -lclangAnalysis \
		-lclangRewrite -lclangRewriteFrontend \
		-lclangEdit -lclangAST -lclangASTMatchers -lclangLex -lclangBasic \
		`llvm-config --libs` -Wl,-Bdynamic `llvm-config --system-libs`

%.o : $(SRC_DIR)/%.cpp 
	$(CXX) -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

h5cpp: h5cpp.o 
	$(CXX)  $^ $(LDFLAGS) $(LIBS)  -o $@	
upload: h5cpp
	scp -i ~/.ssh/analytics.pem h5cpp ubuntu@cluster.h5cpp.ca:/tmp/h5cpp

clean:
	@$(RM) *.o  $(apps)

install: h5cpp
	cp -rf ../h5cpp-llvm $(PREFIX)/include
	cp -rf h5cpp $(PREFIX)/bin
	cp ../h5cpp.pc  $(PREFIX)/lib/pkgconfig

.PHONY: test

