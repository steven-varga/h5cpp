# Copyright (c) 2018 vargaconsulting, Toronto,ON Canada
# Author: Varga, Steven <steven@vargaconsulting.ca>

all: Doxyfile
	@doxygen Doxyfile
	mkdir -p  ../docs/html/cgi
	cp redirect.py ../docs/html/cgi/
test:

upload: all
	rsync -a  ../docs/html/ osaka.vargaconsulting.ca:h5cpp.ca
	rsync -a  ../docs/pix/ osaka.vargaconsulting.ca:h5cpp.ca/pix
view: all
	xdg-open ../docs/html/index.html
clean:
	$(RM) -rf ../docs/html ../docs/latex



