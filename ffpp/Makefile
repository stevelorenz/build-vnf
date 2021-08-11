#
# Makefile
#

# TODO<Zuo>: Use meson to manage sources in ./kern with e.g. custom_target or native methods.

all: release

TARGET_DIR = build
# INFO: Can not be compiled using clang currently since some dependencies (e.g. pybind11) can not be compiled.
CC = gcc
CXX = g++

mk_build_dir:
	if [ ! -d $(TARGET_DIR) ]; then mkdir $(TARGET_DIR); fi

release: mk_build_dir
	CC=$(CC) CXX=$(CXX) meson $(TARGET_DIR) --buildtype=release 
	cd $(TARGET_DIR) && ninja
	cd ./kern && make

debug: mk_build_dir
	CC=$(CC) CXX=$(CXX) meson -Db_sanitize=address,undefined $(TARGET_DIR) --buildtype=debug
	cd $(TARGET_DIR) && ninja
	cd ./kern && make DEBUG=1

examples: release
	meson configure -Dexamples=true $(TARGET_DIR)
	cd $(TARGET_DIR) && ninja

related_works: release
	meson configure -Drelated_works=true $(TARGET_DIR)
	cd $(TARGET_DIR) && ninja

tests: release
	meson configure -Dtests=true $(TARGET_DIR) --buildtype=release
	cd $(TARGET_DIR) && ninja

tests-debug: debug
	meson configure -Dtests=true -Db_coverage=true -Db_sanitize=address,undefined $(TARGET_DIR) --buildtype=debug
	cd $(TARGET_DIR) && ninja

# Run all auto unit tests
# (Some tests can not be checked with ASAN and valgrind due to issues in the dependencies...)
# cd $(TARGET_DIR) && ASAN_OPTIONS=detect_leaks=0 meson test --suite no-leak --print-errorlogs
run-tests:
	@echo "* Run tests:"
	cd $(TARGET_DIR) && meson test --suite no-leak --print-errorlogs

run-tests-v:
	@echo "* Run tests:"
	cd $(TARGET_DIR) && meson test --suite no-leak --print-errorlogs --verbose

run-tests-dev: 
	@echo "* Run tests (under development):"
	cd $(TARGET_DIR) && meson test --suite dev  --print-errorlogs --verbose

# Run tests that currently have memory leaks (Could because of used dependencies).
run-tests-leak:
	cd $(TARGET_DIR) && meson test --suite leak --print-errorlogs --verbose

run-coverage:
	cd $(TARGET_DIR) && ninja test && ninja coverage-html

install: release
	cd $(TARGET_DIR) && ninja install

uninstall:
	cd $(TARGET_DIR) && ninja uninstall

clean:
	rm -rf $(TARGET_DIR)
	cd ./kern && make clean

# Static checkers and formatters for utility scripts, used to run them inside terminal

BASH_SRCS := $(shell find ./ -name '*.sh')
PY_SRCS := $(shell find ./ -name '*.py')
ALL_SCRIPT_SRCS := $(BASH_SRCS) $(PY_SRCS)

codecheck-scripts: $(ALL_SCRIPT_SRCS)
	@echo "* Run flake8 for Python sources..."
	flake8 --ignore=E501,E266,E203,E231,W503,F401,F841 --max-complexity 10 $(PY_SRCS) || true
	@echo "* Run shellcheck for BASH sources..."
	shellcheck $(BASH_SRCS)

format-scripts: $(ALL_SCRIPT_SRCS)
	@echo "* Format all Python sources with black..."
	black $(PY_SRCS)  || true
	@echo "* Format all Bash sources with shfmt..."
	shfmt -i 4 -w $(BASH_SRCS)

C_SRCS := $(shell find ./ -name "*.c")
C_HEADERS := $(shell find ./ -name "*.h")
ALL_C_FILES := $(C_SRCS) $(C_HEADERS)
CPP_SRCS := $(shell find ./ -name "*.cpp")
CPP_HEADERS := $(shell find ./ -name "*.hpp")
ALL_CPP_FILES := $(CPP_SRCS) $(CPP_HEADERS)

codecheck-cxx: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Run cppcheck: "

flawfinder-cxx: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Check torrential flaws and vulnerabilities with static checker:"
	flawfinder --minlevel 2 $(ALL_C_FILES) $(ALL_CPP_FILES)

format-cxx: $(ALL_C_FILES) $(ALL_CPP_FILES)
	@echo "* Format all C sources with clang-format"
	clang-format --style=file -i $(ALL_C_FILES) $(ALL_CPP_FILES)

.PHONY: codecheck-scripts format-scripts codecheck-cxx flawfinder-cxx format-cxx

# Cleanup utilities

rm-all-containers:
	@echo "Remove all docker containers"
	docker container rm $$(docker ps -aq) -f

rm-dangling-images:
	@echo "Remove all dangling docker images"
	docker rmi $$(docker images -f "dangling=true" -q)

pp-empty-dirs:
	@echo "Print empty directories"
	@find -maxdepth 3 -type d -empty

.PHONY: rm-all-containers rm-dangling-images pp-empty-dirs