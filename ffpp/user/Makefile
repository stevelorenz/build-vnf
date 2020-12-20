#
# A wrapper for some commands of the Meson build system.
#

all: release

TARGET_DIR = build

release:
	meson $(TARGET_DIR) --buildtype=release 
	cd $(TARGET_DIR) && ninja

debug:
	if [ ! -d $(TARGET_DIR) ]; then mkdir $(TARGET_DIR); fi
	meson -Db_sanitize=address,undefined $(TARGET_DIR) --buildtype=debug
	cd $(TARGET_DIR) && ninja

examples:
	meson configure -Dexamples=true $(TARGET_DIR)
	cd $(TARGET_DIR) && ninja

related_works:
	meson configure -Drelated_works=true $(TARGET_DIR)
	cd $(TARGET_DIR) && ninja

tests:
	meson configure -Dtests=true -Db_coverage=true -Db_sanitize=address,undefined $(TARGET_DIR) --buildtype=debug
	cd $(TARGET_DIR) && ninja

# Run all auto unit tests
# (Some tests can not be checked with ASAN and valgrind due to issues in the dependencies...)
run-tests:
	@echo "* Run unit tests:"
	cd $(TARGET_DIR) && ASAN_OPTIONS=detect_leaks=0 meson test --suite no-leak --print-errorlogs

run-tests-dev:
	@echo "* Run unit tests (under development):"
	cd $(TARGET_DIR) && meson test --suite dev  --print-errorlogs

# Run tests that currently have memory leaks (Could because of used dependencies).
run-tests-leak:
	cd $(TARGET_DIR) && meson test --suite leak --print-errorlogs

run-coverage:
	cd $(TARGET_DIR) && ninja test && ninja coverage-html

install:
	cd $(TARGET_DIR) && ninja install

uninstall:
	cd $(TARGET_DIR) && ninja uninstall

clean:
	rm -rf $(TARGET_DIR)

.PHONY: release debug examples related_works tests run-tests run-coverage install uninstall clean

C_SRCS := $(shell find ./ -name "*.c")
C_HEADERS := $(shell find ./ -name "*.h")
ALL_C_FILES := $(C_SRCS) $(C_HEADERS)

CPP_SRCS := $(shell find ./ -name "*.cpp")
CPP_HEADERS := $(shell find ./ -name "*.hpp")
ALL_CPP_FILES := $(CPP_SRCS) $(CPP_HEADERS)

cppcheck: $(ALL_C_FILES)
	@echo "* Run cppcheck: "
	cd $(TARGET_DIR) && ninja cppcheck

# WARN: this process takes time.
scan-build: $(ALL_C_FILES)
	@echo "* Run Clang static analyzer:" 
	cd $(TARGET_DIR) && ninja scan-build

flawfinder: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Check torrential flaws and vulnerabilities with static checker:"
	flawfinder --minlevel 2 $(ALL_C_FILES) $(ALL_CPP_FILES)

format: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Format all C sources with clang-format"
	clang-format --style=file -i $(ALL_C_FILES) $(ALL_CPP_FILES)

.PHONY: cppcheck scan-build flawfinder format
