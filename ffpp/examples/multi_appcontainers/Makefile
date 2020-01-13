#
# Makefile
#

DEBUG=1
STRICT=0
include $(RTE_SDK)/mk/rte.vars.mk

CC=gcc
CFLAGS+=-I$(RTE_SDK)/$(RTE_TARGET)/include
CFLAGS+=-I/ffpp/include
LDFLAGS =
LDFLAGS+= -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lpthread -lm -lnuma -ldl
LDFLAGS+= -L/ffpp/build -lffpp -lpcap

OPTS=-O3
ifeq ($(DEBUG), 1)
	OPTS=-O1 -g -DDBUG -fsanitize=undefined
endif

ifeq ($(STRICT), 1)
	# ISSUE: DPDK(19.08) AF_Packet PMD has memory leaks...
	OPTS=-O0 -g -DDBUG -fsanitize=undefined,leak,address -fno-omit-frame-pointer
endif

CFLAGS+=$(OPTS)
CFLAGS+= -mno-rdrnd

all: distributor.out dummy_vnf.out

distributor.out: distributor.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

dummy_vnf.out: dummy_vnf.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY:clean

clean:
	rm -rf *.out *.o
	rm -rf *.log

# vim:ft=make
#
