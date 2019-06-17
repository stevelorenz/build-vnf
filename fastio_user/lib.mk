#
# WARN: This is a temporary build solution, SHOULD be replaced with a better
# solution when I figure out how to ...
#
# - This makefile uses the LEGACY DPDK makefile format, SHOULD use libdpdk-based approach.
# - The paths of sources and headers are hard-coded based on the configuration of the development container described ./Dockerfile and ./util/run_dev_container.sh
#
# About: Makefile for the fastio_user library
#

DEBUG=0

include $(RTE_SDK)/mk/rte.vars.mk

LIB = libfastio_user.so

SRCS-y := $(shell find /fastio_user/src/ -name '*.c')

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

RTE_TARGET ?= x86_64-native-linuxapp-gcc

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -I/fastio_user/include/fastio_user/
CFLAGS += -I/usr/local/include

OPTS=-O3
ifeq ($(DEBUG), 1)
	OPTS=-O0 -g -DDBUG
endif
CFLAGS+=$(OPTS)

ifeq ($(PREFIX), )
	PREFIX := /usr/local
endif

all: $(SRCS-y)

include $(RTE_SDK)/mk/rte.extlib.mk
