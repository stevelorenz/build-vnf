#
# Makefile
#

DEBUG=1
include $(RTE_SDK)/mk/rte.vars.mk

CC=gcc
CFLAGS+=-I$(RTE_SDK)/$(RTE_TARGET)/include
CFLAGS+=-I/ffpp/include
LDFLAGS =
LDFLAGS+= -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lpthread -lm -lnuma -ldl
LDFLAGS+= -L/ffpp/build -lffpp

OPTS=-O3
ifeq ($(DEBUG), 1)
	OPTS=-O0 -g -DDBUG
endif

CFLAGS+=$(OPTS)
CFLAGS+= -mno-rdrnd
# CFLAGS+=-fsanitize=address

all: test_l2_fwd.out test_mp.out test_ring.out test_icmp.out

test_l2_fwd.out: test_l2_fwd.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test_mp.out: test_mp.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test_ring.out: test_ring.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

test_icmp.out: test_icmp.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

.PHONY:clean

clean:
	rm -rf *.out *.o
	rm -rf *.log

# vim:ft=make
#
