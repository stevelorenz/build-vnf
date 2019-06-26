#
# Makefile for FastIO User project
#

SRCS-y := $(shell find ./src/ -name '*.c')
SHARED_HEADERS := $(shell find ./include/fastio_user/ -name '*.h')
PRIVATE_HEADERS := $(shell find ./src/ -name '*.h')
HEADERS := $(SHARED_HEADERS) $(PRIVATE_HEADERS)
DOCS_MD := $(shell find ./docs/ -name '*.md')
EXAMPLES := $(shell find ./examples/ -name '*.c')

lib: $(SRCS-y) $(HEADERS)
	@make -f ./lib.mk all

.PHONY: test mem_check flawfinder clean

test: $(SRCS-y) $(HEADERS)
	@echo "* Run core tests of fastio_user..."
	@cd ./tests/ && make clean > /dev/null && make > /dev/null
	@cd ./tests/ && ./run_tests.py

mem_check: $(SRCS-y) $(HEADERS)
	@echo "* Run core tests with memory leak checking... (WARN: it takes time...)"
	@cd ./tests/ && make clean > /dev/null && make > /dev/null
	@cd ./tests/ && ./run_tests.py --mem_check

flaw_check: $(SRCS-y) $(HEADERS) $(EXAMPLES)
	@echo "* Check torrential flaws and vulnerabilities with static checker..."
	@flawfinder $(SRCS-y) $(EXAMPLES)

clean:
	@echo "* Run cleanups"
	@echo "- cleanup fastio_user library"
	@make -f ./lib.mk clean
	@rm -r ./build/

.PHONY: docs
docs: $(DOCS_MD) $(SRCS-y) $(HEADERS)
	doxygen ./docs/Doxyfile
