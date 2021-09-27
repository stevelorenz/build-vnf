# TODO:  <27-09-21, Zuo> Use Meson to manage sources in ./kern!
all: release

TARGET_DIR = build
# INFO: Can not be compiled using clang currently since some dependencies (e.g. pybind11) can not be compiled.
CC = gcc
CXX = g++
MK_DIR = ./mk

mk_build_dir:
	@if [ ! -d $(TARGET_DIR) ]; then mkdir $(TARGET_DIR); fi

release: mk_build_dir
	@echo "* Build the libraries with release mode"
	CC=$(CC) CXX=$(CXX) meson $(TARGET_DIR) --buildtype=release
	cd $(TARGET_DIR) && ninja
	cd ./kern && make

examples: release
	@echo "* Build examples"
	meson configure -Dexamples=true $(TARGET_DIR)
	cd $(TARGET_DIR) && ninja

related_works: release
	@echo "* Build related works"
	meson configure -Drelated_works=true $(TARGET_DIR)
	cd $(TARGET_DIR) && ninja

tests: release
	@echo "* Build tests with release mode"
	meson configure -Dtests=true $(TARGET_DIR) --buildtype=release
	cd $(TARGET_DIR) && ninja

install: release
	@echo "* Install files"
	cd $(TARGET_DIR) && ninja install

uninstall:
	@echo "* Uninstall files"
	cd $(TARGET_DIR) && ninja uninstall

clean:
	@echo "* Cleanup project"
	rm -rf $(TARGET_DIR)
	cd ./kern && make clean

include $(MK_DIR)/dev.mk
include $(MK_DIR)/static_check.mk
