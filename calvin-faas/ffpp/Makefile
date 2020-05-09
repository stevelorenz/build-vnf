#
# For development and tests of FFPP user and kern libraries.
#

C_SRCS_USER := $(shell find ./user/src/ -name '*.c')
SHARED_HEADERS_USER := $(shell find ./user/include/ -name '*.h')
PRIVATE_HEADERS_USER := $(shell find ./user/src/ -name '*.h')
EXAMPLES_USER := $(shell find ./user/examples/ -name '*.c')
TESTS_USER := $(shell find ./user/tests/ -name '*.c')
DOCS_MD_USER := $(shell find ./user/docs/ -name '*.md')

ALL_HEADERS := $(SHARED_HEADERS_USER) $(PRIVATE_HEADERS_USER)
ALL_SRCS_CH := $(C_SRCS_USER) $(ALL_HEADERS) $(EXAMPLES_USER) $(TESTS_USER)

# Try to automate the building process of both user and kernel space tools.

all: release

release:
	cd ./user && \
		meson -Dc_args=-O3 build && \
		cd ./build && ninja

release-with-tests:
	cd ./user && \
		meson -Dc_args=-O3 -Dtests=true build \
		&& cd ./build && ninja

debug:
	cd ./user && \
		meson -Dc_args=-Og -Dtests=true -Db_coverage=true -Db_sanitize=address build \
		&& cd ./build && ninja

test:
	cd ./user/build && ninja test && ninja coverage-html

install:
	cd ./user/build && ninja install

uninstall:
	cd ./user/build && ninja uninstall

clean:
	cd ./user && rm -r ./build


docs: $(DOCS_MD_USER) $(C_SRCS_USER) $(ALL_HEADERS)
	cd ./user && doxygen ./docs/Doxyfile

.PHONY: release release-with-tests debug docs

# Following actions are ONLY used for static code analysis.

codecheck: $(ALL_SRCS_CH)
	@echo "* Run static checkers..."
	@echo "- Run cppcheck all: "
	cppcheck --enable=all $(ALL_SRCS_CH)

flawcheck: $(ALL_SRCS_CH)
	@echo "* Check torrential flaws and vulnerabilities with static checker..."
	flawfinder --minlevel 2 $(ALL_SRCS_CH)

format: $(ALL_SRCS_CH)
	@echo "* Format all C sources with clang-format"
	clang-format -i $(ALL_SRCS_CH)

.PHONY: codecheck flawcheck format
