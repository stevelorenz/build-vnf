# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -Wno-unused-but-set-parameter
CFLAGS += -Wno-deprecated-declarations

# External libraries
APP_PATH = /vagrant/vnf_release/dpdk_udp_nc
CFLAGS += -I${APP_PATH}
DPDK_HELPER_PATH= /vagrant/shared_lib/
CFLAGS += -I${DPDK_HELPER_PATH}
NCKERNEL_PATH= /vagrant/vm_share/nckernel/
CFLAGS += -I${NCKERNEL_PATH}/include
LDLIBS += -L${NCKERNEL_PATH}/lib -lnckernel

# binary name
APP = test

# all source are stored in SRCS-y
SRCS-y := test.c

CFLAGS += -O3

include $(RTE_SDK)/mk/rte.extapp.mk
