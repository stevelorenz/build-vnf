all: release

release:
	cd ./common/ && make
	cd ./xdp_drop/ && make
	cd ./xdp_count/ && make

debug:
	cd ./common/ && make DEBUG=1
	cd ./xdp_drop/ && make DEBUG=1
	cd ./xdp_count/ && make DEBUG=1

clean:
	cd ./common/ && make clean
	cd ./xdp_drop/ && make clean
	cd ./xdp_count/ && make clean

# ONLY used by the developers.

C_XDP_SRCS := $(shell find ./ -name '*.c')
C_XDP_SHARED_HEADERS := $(shell find ./ -name '*.h')
C_ALL_SRCS := $(C_XDP_SRCS)
C_ALL_HEADERS := $(C_XDP_SHARED_HEADERS)
C_ALL_SRCS_AND_HEADERS := $(C_ALL_SRCS) $(C_ALL_HEADERS)

flawcheck: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Check torrential flaws and vulnerabilities with static checker..."
	flawfinder --minlevel 2 $(C_ALL_SRCS_AND_HEADERS)

format: $(C_ALL_SRCS_AND_HEADERS)
	@echo "* Format all C sources with clang-format"
	clang-format --style=file -i $(C_ALL_SRCS_AND_HEADERS)

.PHONY: flawcheck format
