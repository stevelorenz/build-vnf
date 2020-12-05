#
# A wrapper for some commands of the Meson build system.
#

all: release

release:
	meson build --buildtype=release 
	cd ./build && ninja

debug:
	meson -Db_sanitize=address,undefined build --buildtype=debug
	cd ./build && ninja

examples:
	meson configure -Dexamples=true build
	cd ./build/ && ninja

related_works:
	meson configure -Drelated_works=true build
	cd ./build/ && ninja

tests:
	meson configure -Dtests=true -Db_coverage=true -Db_sanitize=address,undefined build --buildtype=debug
	cd ./build/ && ninja

# Run all auto unit tests
# (Some tests can not be checked with ASAN and valgrind due to issues in the dependencies...)
run-tests:
	@echo "* Run unit tests:"
	cd ./build && ASAN_OPTIONS=detect_leaks=0 meson test --suite no-leak --print-errorlogs

run-tests-dev:
	@echo "* Run unit tests (under development):"
	cd ./build && meson test --suite dev  --print-errorlogs

# Run tests that currently have memory leaks (Could because of used dependencies).
run-tests-leak:
	cd ./build/ && meson test --suite leak --print-errorlogs

run-coverage:
	cd ./build && ninja test && ninja coverage-html

install:
	cd ./build && ninja install

uninstall:
	cd ./build && ninja uninstall

clean:
	rm -rf ./build

.PHONY: release debug examples related_works tests run-tests run-coverage install uninstall clean

C_SRCS := $(shell find ./ -name "*.c")
C_HEADERS := $(shell find ./ -name "*.h")
ALL_C_FILES := $(C_SRCS) $(C_HEADERS)

CPP_SRCS := $(shell find ./ -name "*.cpp")
CPP_HEADERS := $(shell find ./ -name "*.hpp")
ALL_CPP_FILES := $(CPP_SRCS) $(CPP_HEADERS)

cppcheck: $(ALL_C_FILES)
	@echo "* Run cppcheck: "
	cd ./build && ninja cppcheck

# WARN: this process takes time.
scan-build: $(ALL_C_FILES)
	@echo "* Run Clang static analyzer:" 
	cd ./build && ninja scan-build

flawfinder: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Check torrential flaws and vulnerabilities with static checker:"
	flawfinder --minlevel 2 $(ALL_C_FILES) $(ALL_CPP_FILES)

format: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Format all C sources with clang-format"
	clang-format --style=file -i $(ALL_C_FILES) $(ALL_CPP_FILES)

.PHONY: cppcheck scan-build flawfinder format
