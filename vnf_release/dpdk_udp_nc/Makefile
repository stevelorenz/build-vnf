# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# Binary name
APP = udp_nc

# All source are stored in SRCS-y
SRCS-y := l2fwd.c ncmbuf.c kni.c

# Build using pkg-config variables if possible
$(shell pkg-config --exists libdpdk)
ifeq ($(.SHELLSTATUS),0)

# Build using libdpdk
all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	ln -sf $(APP)-shared build/$(APP)
	static: build/$(APP)-static
	ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell pkg-config --path libdpdk)
CFLAGS += -O3 $(shell pkg-config --cflags libdpdk)
LDFLAGS_SHARED = $(shell pkg-config --libs libdpdk)
LDFLAGS_STATIC = -Wl,-Bstatic $(shell pkg-config --static --libs libdpdk)

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	rmdir --ignore-fail-on-non-empty build

else # Build using legacy build system

ifeq ($(RTE_SDK),)
	$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overridden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

CFLAGS += -O3
CFLAGS += $(WERROR_FLAGS)
CFLAGS += -Wno-unused-but-set-parameter
CFLAGS += -Wno-deprecated-declarations
#EXTRA_CFLAGS += -O3

# Set NCKernel codec parameters
CFLAGS += -DSYMBOL_SIZE=\"1402\" -DSYMBOLS=\"2\" -DREDUNDANCY=\"1\"

# Debugging and profile
# - gprof
# - address sanitizer
EXTRA_CFLAGS += -O1 -pg -g -fsanitize=address

# External libraries

NCKERNEL_PATH= /vagrant/vm_share/nckernel/
CFLAGS += -I${NCKERNEL_PATH}/include
LDLIBS += -L${NCKERNEL_PATH}/lib -lnckernel

DPDKHELPER_PATH= /vagrant/shared_lib/dpdk_helper
CFLAGS += -I${DPDKHELPER_PATH}
LDLIBS += -L${DPDKHELPER_PATH}/build -ldpdkhelper

include $(RTE_SDK)/mk/rte.extapp.mk
endif
