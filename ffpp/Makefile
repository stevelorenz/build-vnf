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

.PHONY: install uninstall clean

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
