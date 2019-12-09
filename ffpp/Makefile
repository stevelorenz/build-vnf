#
# Makefile for FFPP library development
#

SRCS-y := $(shell find ./src/ -name '*.c')
SHARED_HEADERS := $(shell find ./include/ffpp/ -name '*.h')
PRIVATE_HEADERS := $(shell find ./src/ -name '*.h')
HEADERS := $(SHARED_HEADERS) $(PRIVATE_HEADERS)
DOCS_MD := $(shell find ./docs/ -name '*.md')
EXAMPLES := $(shell find ./examples/ -name '*.c')
TESTS := $(shell find ./tests/ -name '*.c')
ALL_SRCS_CH := $(SRCS-y) $(HEADERS) $(EXAMPLES) $(TESTS)

# Release version
lib: $(SRCS-y) $(HEADERS)
	@make -f ./lib.mk all

lib_debug: $(SRCS-y) $(HEADERS)
	@make DEBUG=1 -f ./lib.mk all

.PHONY: test memcheck flawcheck codecheck format clean

test: $(SRCS-y) $(HEADERS)
	@echo "* Run core tests of ffpp..."
	@cd ./tests/ && make clean > /dev/null && make > /dev/null
	@cd ./tests/ && ./run_tests.py

memcheck: $(SRCS-y) $(HEADERS)
	@echo "* Run core tests with memory leak checking... (WARN: it takes time...)"
	@cd ./tests/ && make clean > /dev/null && make > /dev/null
	@cd ./tests/ && ./run_tests.py --mem_check

flawcheck: $(ALL_SRCS_CH)
	@echo "* Check torrential flaws and vulnerabilities with static checker..."
	@flawfinder $(ALL_SRCS_CH)

codecheck: $(ALL_SRCS_CH)
	@echo "* Run static checkers..."
	@echo "- Run cppcheck all: "
	@cppcheck --enable=all $(ALL_SRCS_CH)

format: $(ALL_SRCS_CH)
	@echo "* Format all C sources with clang-format"
	@clang-format -i $(ALL_SRCS_CH)

clean:
	@echo "* Run cleanups"
	@echo "- cleanup ffpp library"
	@make -f ./lib.mk clean
	@rm -r ./build/

.PHONY: docs
docs: $(DOCS_MD) $(SRCS-y) $(HEADERS)
	doxygen ./docs/Doxyfile
