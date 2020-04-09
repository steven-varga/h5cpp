#  _________________________________________________________
#  Copyright (c) 2018-2020 Steven Varga, Toronto,ON Canada
#  Author: Varga, Steven <steven@vargaconsulting.ca>
#  _________________________________________________________

DIRS = attributes basics compound csv datasets datatypes half-float/christian-rau linalg \
	multi-tu optimized packet-table raw_memory stl string transform utf
#to yank: groups kita mpi sparse 
#basics compound packet-table  raw_memory linalg transform multi-tu stl utf

BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
DISTCLEANDIRS = $(DIRS:%=dist-clean-%)
TESTDIRS = $(DIRS:%=test-%)

all: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: 

test: $(TESTDIRS) all
$(TESTDIRS): 
	$(MAKE) -C $(@:test-%=%) test

clean: $(CLEANDIRS)
$(CLEANDIRS): 
	$(MAKE) -C $(@:clean-%=%) clean

dist-clean: $(DISTCLEANDIRS)
$(DISTCLEANDIRS): 
	$(MAKE) -C $(@:dist-clean-%=%) dist-clean

.PHONY: subdirs $(DIRS)
.PHONY: subdirs $(BUILDDIRS)
.PHONY: subdirs $(TESTDIRS)
.PHONY: subdirs $(CLEANDIRS)
.PHONY: all install clean tests
