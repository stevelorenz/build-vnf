#
# Makefile
#

DEBUG=0
include $(RTE_SDK)/mk/rte.vars.mk

CC=gcc
CFLAGS+=-I$(RTE_SDK)/$(RTE_TARGET)/include
CFLAGS+=-I/fpp/include
LDFLAGS =
LDFLAGS+= -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lpthread -lm -lnuma -ldl
LDFLAGS+= -L/fpp/build -lfpp

OPTS=-O3
ifeq ($(DEBUG), 1)
	OPTS=-O0 -g -DDBUG
endif

CFLAGS+=$(OPTS)

all: latency_benchmark_local.out

latency_benchmark_local.out: latency_benchmark_local.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY:clean

clean:
	rm -rf *.out *.o
	rm -rf *.log

# vim:ft=make
#
