#
# Makefile for FastIO User project
#

SRCS-y := $(shell find ./src/ -name '*.c')
HEADERS := $(shell find ./include/fastio_user/ -name '*.h')
DOCS_MD := $(shell find ./docs/ -name '*.md')

lib: $(SRCS-y) $(HEADERS)
	@make -f ./lib.mk all

.PHONY: test
test: $(SRCS-y) $(HEADERS)
	@echo "* Run core tests of fastio_user..."
	@cd ./tests/ && make clean > /dev/null && make > /dev/null
	@cd ./tests/ && ./run_tests.sh

.PHONY: mem_check
mem_check: $(SRCS-y) $(HEADERS)
	@echo "* Run core tests with memory leak checking... (WARN: it takes time...)"
	@cd ./tests/ && make clean > /dev/null && make > /dev/null
	@cd ./tests/ && ./run_tests.sh -c

clean:
	@echo "* Run cleanups"
	@echo "- cleanup fastio_user library"
	@make -f ./lib.mk clean
	@rm -r ./build/

.PHONY: docs
docs: $(DOCS_MD) $(SRCS-y) $(HEADERS)
	doxygen ./docs/Doxyfile
