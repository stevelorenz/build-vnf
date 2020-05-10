C_FFPP_SRCS := $(shell find ./src/ -name '*.c')
C_FFPP_SHARED_HEADERS := $(shell find ./include/ -name '*.h')
C_FFPP_PRIVATE_HEADERS := $(shell find ./src/ -name '*.h')
C_EXAMPLES_SRCS := $(shell find ./examples/ -name '*.c')
C_TESTS_SRCS := $(shell find ./tests/ -name '*.c')
C_ALL_SRCS := $(C_FFPP_SRCS) $(C_TESTS_SRCS) $(C_EXAMPLES_SRCS)
C_ALL_HEADERS := $(C_FFPP_SHARED_HEADERS) $(C_FFPP_PRIVATE_HEADERS)
C_ALL_SRCS_AND_HEADERS := $(C_ALL_SRCS) $(C_ALL_HEADERS)

codecheck: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Run basic static checkers..."
	@echo "- Run cppcheck: "
	cppcheck --enable=warning,style,performance,information --force \
		-I /usr/local/include $(C_ALL_SRCS_AND_HEADERS)

clang-scan: $(C_ALL_SRCS_AND_HEADERS)
	cd ./build && ninja scan-build

flawcheck: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Check torrential flaws and vulnerabilities with static checker..."
	flawfinder --minlevel 2 $(C_ALL_SRCS_AND_HEADERS)

format: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Format all C sources with clang-format"
	clang-format --style=file -i $(C_ALL_SRCS_AND_HEADERS)

.PHONY: codecheck flawcheck format clang-scan
