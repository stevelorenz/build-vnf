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

all: load_gen_image.out

load_gen_image.out: load_gen_image.c
	$(CC) $(CFLAGS) load_gen_image.c -o load_gen_image.out $(LDFLAGS)


.PHONY:clean

clean:
	rm -rf *.out *.o

# vim:ft=make
#
