#
# Makefile to automate build processes of both user and kernel space sources.
#

all: release

release:
	cd ./user && \
		meson -Dc_args=-O3 build && \
		cd ./build && ninja
	cd ./kern/xdp_pass/ && make

debug:
	cd ./user && \
		meson -Dc_args=-Og -Dtests=true -Db_coverage=true -Db_sanitize=address build \
		&& cd ./build && ninja
	cd ./kern/xdp_pass/ && make DEBUG=1

test:
	cd ./user/build && ninja test && ninja coverage-html

install:
	cd ./user/build && ninja install

uninstall:
	cd ./user/build && ninja uninstall

clean:
	cd ./user && rm -r ./build
	cd ./kern/xdp_pass && make clean

docs:
	cd ./user && doxygen ./docs/Doxyfile

.PHONY: release release-with-tests debug docs
