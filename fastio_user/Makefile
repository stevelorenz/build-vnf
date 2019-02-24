#
# About: Makefile for the fastio_user library
#
# MARK :
# - Run make command in the container descriped in ./Dockerfile.dpdk for all dependencies
# - Currently uses the LEGACY DPDK makefile format, should use libdpdk-based approach
# - Use Cmake latter
#

include $(RTE_SDK)/mk/rte.vars.mk

LIB = libfastio_user.so

SRCS-y := ./src/utils.c

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

RTE_TARGET ?= x86_64-native-linuxapp-gcc

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -I/fastio_user/include/fastio_user/

ifeq ($(PREFIX), )
	PREFIX := /usr/local
endif

include $(RTE_SDK)/mk/rte.extlib.mk
