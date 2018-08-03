include $(RTE_SDK)/mk/rte.vars.mk

LIB = libdpdkhelper.so

SRCS-y := dpdk_helper.c

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

RTE_TARGET ?= x86_64-native-linuxapp-gcc

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)

ifeq ($(PREFIX), )
	PREFIX := /usr/local
endif

install: build/lib/libdpdkhelper.so
	sudo install -d $(DESTDIR)$(PREFIX)/lib/
	sudo install -m 644 build/lib/libdpdkhelper.so $(DESTDIR)$(PREFIX)/lib/

include $(RTE_SDK)/mk/rte.extlib.mk
