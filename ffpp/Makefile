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


# Static checkers and formatters for utility scripts
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

.PHONY: clean docs codecheck-scripts format-scripts

## Cleanup utilities

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
