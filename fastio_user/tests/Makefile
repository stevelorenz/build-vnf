#
# Makefile
#

DEBUG=1
include $(RTE_SDK)/mk/rte.vars.mk

CC=gcc
CFLAGS+=-I$(RTE_SDK)/$(RTE_TARGET)/include
CFLAGS+=-I/fastio_user/include
LDFLAGS =
LDFLAGS+= -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lpthread -lm -lnuma -ldl
LDFLAGS+= -L/fastio_user/build -lfastio_user

OPTS=-O3
ifeq ($(DEBUG), 1)
	OPTS=-O0 -g -DDBUG
endif

CFLAGS+=$(OPTS)

all: test_l2_fwd.out test_mp.out test_ring.out

test_l2_fwd.out: test_l2_fwd.c
	$(CC) $(CFLAGS) test_l2_fwd.c -o test_l2_fwd.out $(LDFLAGS)

test_mp.out: test_mp.c
	$(CC) $(CFLAGS) test_mp.c -o test_mp.out $(LDFLAGS)

test_ring.out: test_ring.c
	$(CC) $(CFLAGS) test_ring.c -o test_ring.out $(LDFLAGS)

.PHONY:clean

clean:
	rm -rf *.out *.o
	rm -rf *.log

# vim:ft=make
#
