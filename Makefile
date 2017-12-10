#  _____________________________________________________________________________
#
#  Copyright (c) <2015> <copyright Steven Istvan Varga, Toronto, On>
#
#  Contact: Steven Varga
#           steven.varga@gmail.com
#           2015 Toronto, On Canada
#  _____________________________________________________________________________
#

PREFIX = /usr/local

DIRS =  tests profiling examples doxy

BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
TESTDIRS = $(DIRS:%=test-%)

all: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: 
	cp -rf h5cpp $(PREFIX)/include
	cp h5cpp.pc  $(PREFIX)/lib/pkgconfig

test: $(TESTDIRS) all
$(TESTDIRS): 
	$(MAKE) -C $(@:test-%=%) test

clean: $(CLEANDIRS)
$(CLEANDIRS): 
	$(MAKE) -C $(@:clean-%=%) clean

.PHONY: subdirs $(DIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(TESTDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all install clean tests


