#  _____________________________________________________________________________
#
#  Copyright (c) <2018> <copyright Steven Varga, Toronto, On>
#
#  Contact: Steven Varga
#           steven@vargaconsulting.ca
#           2018 Toronto, On Canada
#  _____________________________________________________________________________
#

PREFIX = /usr/local

DIRS =  tests doxy examples profile/grid_engine profile/gperf compiler

BUILDDIRS = $(DIRS:%=build-%)
CLEANDIRS = $(DIRS:%=clean-%)
TESTDIRS  = $(DIRS:%=test-%)

all: $(BUILDDIRS)
$(DIRS): $(BUILDDIRS)
$(BUILDDIRS):
	$(MAKE) -C $(@:build-%=%)

install: 
	cp -rf h5cpp $(PREFIX)/include
	cp -rf h5cpp-llvm $(PREFIX)/include
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

upload:
	ssh ubuntu@master "rm -fr /usr/local/include/h5cpp"
	ssh ubuntu@master "sudo rm -fr /tmp/examples"
	scp -r h5cpp ubuntu@master:/usr/local/include/
	scp -r examples ubuntu@master:/tmp/ 
	ssh ubuntu@master "sudo chown -R test001:test001 /tmp/examples"
