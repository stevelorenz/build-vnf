#
# Makefile to automate build processes of both user and kernel space sources.
#

all: release

release:
	cd ./user && make release
	cd ./kern && make

debug:
	cd ./user && make debug
	cd ./kern && make DEBUG=1

install:
	cd ./user/ && make install

uninstall:
	cd ./user/ && make uninstall

clean:
	cd ./user && make clean
	cd ./kern && make clean

docs:
	cd ./user && doxygen ./docs/Doxyfile

.PHONY: release debug install uninstall clean docs
