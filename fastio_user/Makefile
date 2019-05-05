#
# About: Makefile for the fastio_user library
#
# MARK :
# - Run make command in the container descriped in ./Dockerfile.dpdk for all dependencies
# - Currently uses the LEGACY DPDK makefile format, should use libdpdk-based approach
# - Use Cmake latter

DEBUG=1

include $(RTE_SDK)/mk/rte.vars.mk

LIB = libfastio_user.so

SRCS-y := $(addprefix ./src/, utils.c memory.c device.c io.c task.c ipc.c collections.c \
	mp.c)

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

RTE_TARGET ?= x86_64-native-linuxapp-gcc

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -I/fastio_user/include/fastio_user/
CFLAGS += -I/usr/local/include
CFLAGS += -I/stb

OPTS=-O3
ifeq ($(DEBUG), 1)
	OPTS=-O0 -g -DDBUG
endif
CFLAGS+=$(OPTS)

ifeq ($(PREFIX), )
	PREFIX := /usr/local
endif

all: $(SRCS-y)

test: all
	@echo "* Run core tests of fastio_user..."
	@cd ./tests/ && make clean > /dev/null && make > /dev/null
	@cd ./tests/ && ./run_tests.sh

include $(RTE_SDK)/mk/rte.extlib.mk
