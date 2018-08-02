include $(RTE_SDK)/mk/rte.vars.mk

LIB = libdpdkhelper.so

SRCS-y := dpdk_helper.c

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

RTE_TARGET ?= x86_64-native-linuxapp-gcc

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)

include $(RTE_SDK)/mk/rte.extlib.mk
