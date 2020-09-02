#
# Makefile
#

BASH_SRCS := $(shell find ./ -name '*.sh')
PY_SRCS := $(shell find ./ -name '*.py')
ALL_SCRIPT_SRCS := $(BASH_SRCS) $(PY_SRCS)

all:
	@echo "Makefile needs your attention"

codecheck-scripts: $(ALL_SCRIPT_SRCS)
	@echo "* Run flake8 for Python sources..."
	flake8 --ignore=E501,E266,E203,E231,W503,F401,F841 $(PY_SRCS)
	@echo "* Run shellcheck for BASH sources..."
	shellcheck $(BASH_SRCS)

format-scripts: $(ALL_SCRIPT_SRCS)
	@echo "* Format all Python sources with black..."
	black $(PY_SRCS)
	@echo "* Format all Bash sources with shfmt..."
	shfmt -i 4 -w $(BASH_SRCS)

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


# vim:ft=make
#
