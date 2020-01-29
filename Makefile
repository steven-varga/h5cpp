
PREFIX = /usr/local

<<<<<<< HEAD
VERSION = 1.10.4.4
PROGRAM_NAME=h5cpp-dev
=======
VERSION = 1.10.4.5-1
PROGRAM_NAME=libh5cpp-dev
>>>>>>> 4be3967643f00b8f4929bd9a2cfbc9398a28b332
BIN_DIR = $(PREFIX)/bin
INCLUDE_DIR = $(PREFIX)/include
EXAMPLE_DIR = $(PREFIX)/share

MAN_BASE_DIR = $(PREFIX)/man
MAN_DIR = $(MAN_BASE_DIR)/man1
MAN_EXT = 1


INSTALL = install	# install : UCB/GNU Install compatiable

RM      = rm -f
MKDIR   = mkdir -p

CC ?= gcc
COMPILER_OPTIONS = -Wall -O -g

INSTALL_PROGRAM = $(INSTALL) -c -m 0755
INSTALL_DATA    = $(INSTALL) -c -m 0644
INSTALL_EXAMPLE    = $(INSTALL) -c -m 0755
INSTALL_INCLUDE = $(INSTALL)    -m 0755

OBJECT_FILES = 
#fdupes.o md5/md5.o $(ADDITIONAL_OBJECTS)
<<<<<<< HEAD

#####################################################################
# no need to modify anything beyond this point                      #
#####################################################################

all: 

h5cpp-dev: 

installdirs:
	test -d $(INCLUDE_DIR) || $(MKDIR) $(MAN_DIR)

install: installdirs
	echo "********  $(INCLUDE_DIR) *******"
	$(INSTALL_INCLUDE)	-d $(INCLUDE_DIR)/h5cpp
	find h5cpp -type f -exec install -Dm 755 "{}" "${INCLUDE_DIR}/{}" \;
	$(INSTALL_EXAMPLE)  -d $(EXAMPLE_DIR)/h5cpp
	find examples -type f -exec install -Dm 644 "{}" "${EXAMPLE_DIR}/h5cpp/{}" \;

clean:

dist: h5cpp-dev
	debuild -i -us -uc -b
=======

#####################################################################
# no need to modify anything beyond this point                      #
#####################################################################

all: 

libh5cpp-dev: 

installdirs:
	test -d $(INCLUDE_DIR) || $(MKDIR) $(MAN_DIR)

install: installdirs
	echo "********  $(INCLUDE_DIR) *******"
	$(INSTALL_INCLUDE)	-d $(INCLUDE_DIR)/h5cpp
	find h5cpp -type f -exec install -Dm 755 "{}" "${INCLUDE_DIR}/{}" \;
	$(INSTALL_EXAMPLE)  -d $(EXAMPLE_DIR)/h5cpp
	find examples -type f -exec install -Dm 644 "{}" "${EXAMPLE_DIR}/h5cpp/{}" \;

tar-gz:
	tar --exclude='.[^/]*' -czvf ../libh5cpp_1.10.4.5.orig.tar.gz ./

clean:
	@$(RM) h5cpp-* 

dist-debian: libh5cpp-dev
	debuild -i -us -uc -b

dist-rpm: dist-debian
	sudo alien -r ../libh5cpp-dev_1.10.4.5-1_all.deb
>>>>>>> 4be3967643f00b8f4929bd9a2cfbc9398a28b332

