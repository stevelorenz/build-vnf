# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

CFLAGS += -O0
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -Wno-unused-but-set-parameter
CFLAGS += -Wno-unused-parameter
CFLAGS += -Wno-deprecated-declarations
CFLAGS += -Wno-unused-but-set-variable

EXTRA_CFLAGS += -O0 -pg -g -fsanitize=address -fprofile-arcs -ftest-coverage

# External libraries
CFLAGS += -I../

NCKERNEL_PATH= /vagrant/vm_share/nckernel/
CFLAGS += -I${NCKERNEL_PATH}/include
LDLIBS += -L${NCKERNEL_PATH}/lib -lnckernel

APP_PATH = /vagrant/vnf_release/dpdk_udp_nc/
CFLAGS += -I${APP_PATH}

DPDKHELPER_PATH= /vagrant/shared_lib/dpdk_helper
CFLAGS += -I${DPDKHELPER_PATH}
LDLIBS += -L${DPDKHELPER_PATH}/build -ldpdkhelper

# Binary name
APP = test

# All source are stored in SRCS-y
SRCS-y := test.c ../ncmbuf.c ../kni.c

include $(RTE_SDK)/mk/rte.extapp.mk
