#
# For FFPP library development and tests.
#

C_SRCS := $(shell find ./src/ -name '*.c')
SHARED_HEADERS := $(shell find ./include/ffpp/ -name '*.h')
PRIVATE_HEADERS := $(shell find ./src/ -name '*.h')
HEADERS := $(SHARED_HEADERS) $(PRIVATE_HEADERS)
DOCS_MD := $(shell find ./docs/ -name '*.md')
EXAMPLES := $(shell find ./examples/ -name '*.c')
TESTS := $(shell find ./tests/ -name '*.c')
ALL_SRCS_CH := $(C_SRCS) $(HEADERS) $(EXAMPLES) $(TESTS)

.PHONY: codecheck codecheck format

codecheck: $(ALL_SRCS_CH)
	@echo "* Run static checkers..."
	@echo "- Run cppcheck all: "
	@cppcheck --enable=all $(ALL_SRCS_CH)

flawcheck: $(ALL_SRCS_CH)
	@echo "* Check torrential flaws and vulnerabilities with static checker..."
	@flawfinder --minlevel 2 $(ALL_SRCS_CH)

format: $(ALL_SRCS_CH)
	@echo "* Format all C sources with clang-format"
	@clang-format -i $(ALL_SRCS_CH)

.PHONY: docs
docs: $(DOCS_MD) $(C_SRCS) $(HEADERS)
	doxygen ./docs/Doxyfile

# ONLY used for testing and debugging.
# Assume build directory is already created by `meson build` with default options.

config-tests-debug:
	@meson configure build -Dc_args=-Og -Dtests=true -Db_coverage=true -Db_sanitize=address
