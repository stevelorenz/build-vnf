#
# Makefile
# The path is based on the Dockerfile: ../../dockerfiles/Dockerfile.darknet
# Assume the darknet source is already compiled and located in \root\darknet
#

# Use make DBUG=1 to enable debug mode
DEBUG=0
DPDK=1

VPATH=./
EXEC=yolo_v2_pp.out
OBJ=yolo_v2_pp.o
SRC=yolo_v2_pp.c

CC=gcc
CFLAGS=-Wall -I/root/darknet/include
CFLAGS+=-I$(RTE_SDK)/$(RTE_TARGET)/include
LDFLAGS =
LDFLAGS+= -L/root/darknet/ -ldarknet
LDFLAGS+= -L${RTE_SDK}/${RTE_TARGET}/lib -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -lpthread -lm -lnuma -ldl

OPTS=-O3
ifeq ($(DEBUG), 1)
	OPTS=-O2 -g -DDBUG
endif

ifeq ($(DPDK), 1)
	OPTS+=-DDPDK
endif

CFLAGS+=$(OPTS)

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:clean

clean:
	rm -rf $(EXEC) $(OBJ)


# vim:ft=make
#
