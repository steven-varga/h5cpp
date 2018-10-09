# Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>

apps = h5cpp

PREFIX = /usr/local
CXXFLAGS =  -I../  `llvm-config-7 --cxxflags` -std=c++17 -fno-strict-aliasing
LDFLAGS = `llvm-config-7 --ldflags`
LIBS =  -lclangFrontend -lclangSerialization -lclangDriver \
		-lclangTooling -lclangParse -lclangSema -lclangAnalysis \
		-lclangRewrite -lclangRewriteFrontend \
		-lclangEdit -lclangAST -lclangASTMatchers -lclangLex -lclangBasic \
		`llvm-config-7 --libs` -Wl,-Bdynamic `llvm-config-7 --system-libs`

%.o : $(SRC_DIR)/%.cpp
	$(CXX) -$(INCLUDES) -o $@  $(CPPFLAGS) $(CXXFLAGS) -c $^

all: $(apps)

h5cpp: h5cpp.o
	$(CXX)  $^ $(LDFLAGS)  $(LIBS)  -o $@

clean:
	@$(RM) *.o  $(apps)

install: h5cpp
	cp -rf h5cpp $(PREFIX)/bin
	cp ../h5cpp.pc  $(PREFIX)/lib/pkgconfig

.PHONY: test
