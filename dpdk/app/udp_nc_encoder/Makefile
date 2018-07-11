# About: Makefile for DPDK app udp_encoder
#

APP = udp_nc_encoder

# All source are stored in SRCS-y
SRCS-y := main.c

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overridden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

CFLAGS += -O3
#CFLAGS += $(WERROR_FLAGS)

# Add kodo-c shared library
KODOC_PATH=/home/vagrant/kodo-c
CFLAGS += -I${KODOC_PATH}/
LDLIBS += -L${KODOC_PATH}/ -lkodoc

include $(RTE_SDK)/mk/rte.extapp.mk
