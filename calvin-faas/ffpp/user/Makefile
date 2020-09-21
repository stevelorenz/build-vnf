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

# Build all tests
# ISSUE: Address sanitizer is DISABLED because the AF_PACKET PMD of DPDK has
# memory leaks... Add address back when this is fixed in new DPDK release.
tests:
	meson configure -Dtests=true -Db_coverage=true -Db_sanitize=undefined build --buildtype=debug
	cd ./build/ && ninja

# Run all auto unit tests
# (Some tests can not be checked with ASAN and valgrind due to issues in the dependencies...)
run-tests:
	cd ./build && meson test --print-errorlogs

# Run tests in the dev suite.
run-tests-dev:
	cd ./build/ && meson test --suite dev --print-errorlogs
run-tests-dev-mem:
	cd ./build/ && meson test --wrap='valgrind --leak-check=yes' --suite dev --print-errorlogs

run-coverage:
	cd ./build && ninja test && ninja coverage-html

install:
	cd ./build && ninja install

uninstall:
	cd ./build && ninja uninstall

clean:
	rm -rf ./build

.PHONY: release debug examples related_works tests run-tests run-coverage install uninstall clean

C_FFPP_SRCS := $(shell find ./src/ -name '*.c')
C_FFPP_SHARED_HEADERS := $(shell find ./include/ -name '*.h')
C_FFPP_PRIVATE_HEADERS := $(shell find ./src/ -name '*.h')
C_EXAMPLES_SRCS := $(shell find ./examples/ -name '*.c')
C_TESTS_SRCS := $(shell find ./tests/ -name '*.c')
C_ALL_SRCS := $(C_FFPP_SRCS) $(C_TESTS_SRCS) $(C_EXAMPLES_SRCS)
C_ALL_HEADERS := $(C_FFPP_SHARED_HEADERS) $(C_FFPP_PRIVATE_HEADERS)
C_ALL_SRCS_AND_HEADERS := $(C_ALL_SRCS) $(C_ALL_HEADERS)

codecheck: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Run basic static checkers..."
	@echo "- Run cppcheck: "
	cppcheck --enable=warning,style,performance,information --force \
		-I /usr/local/include $(C_ALL_SRCS_AND_HEADERS)

clang-scan: $(C_ALL_SRCS_AND_HEADERS)
	cd ./build && ninja scan-build

flawcheck: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Check torrential flaws and vulnerabilities with static checker..."
	flawfinder --minlevel 2 $(C_ALL_SRCS_AND_HEADERS)

format: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Format all C sources with clang-format"
	clang-format --style=file -i $(C_ALL_SRCS_AND_HEADERS)

.PHONY: codecheck flawcheck format clang-scan
