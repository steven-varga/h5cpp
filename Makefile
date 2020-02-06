
PREFIX = /usr/local

VERSION = 1.10.4.5
DEB = -1~exp1
PROGRAM_NAME=libh5cpp-dev
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

#####################################################################
# no need to modify anything beyond this point                      #
#####################################################################

all: 


installdirs:
	test -d $(INCLUDE_DIR) || $(MKDIR) $(MAN_DIR)

install: installdirs
	echo "********  $(INCLUDE_DIR) *******"
	$(INSTALL_INCLUDE)	-d $(INCLUDE_DIR)/h5cpp
	find h5cpp -type f -exec install -Dm 755 "{}" "${INCLUDE_DIR}/{}" \;
	$(INSTALL_EXAMPLE)  -d $(EXAMPLE_DIR)/h5cpp
	find examples -type f -exec install -Dm 644 "{}" "${EXAMPLE_DIR}/h5cpp/{}" \;

tar-gz:
	tar --exclude='.[^/]*' --exclude-vcs-ignores -czvf ../libh5cpp_${VERSION}.orig.tar.gz ./ 
	gpg --detach-sign --armor ../libh5cpp_${VERSION}.orig.tar.gz	
	scp ../libh5cpp_${VERSION}.orig.tar.* osaka:h5cpp.org/download/ 
clean:
	@$(RM) h5cpp-* 

dist-debian-src: tar-gz
	debuild -i -us -uc -S

dist-debian-bin:
	debuild -i -us -uc -b

dist-debian-src-upload: dist-debian-src
	debsign -k 1B04044AF80190D78CFBE9A3B971AC62453B78AE ../libh5cpp_${VERSION}${DEB}_source.changes
	dput mentors ../libh5cpp_${VERSION}${DEB}_source.changes

dist-rpm: dist-debian
	sudo alien -r ../libh5cpp-dev_${VERSION}_all.deb

