# SPDX-License-Identifier: (GPL-2.0)
CC := gcc

all: ext_xdp_user_utils.o

CFLAGS := -Wall

XDPTOOLS_DIR = /opt/xdp-tools
CFLAGS += -I$(XDPTOOLS_DIR)/lib/libbpf/include

ext_xdp_user_utils.o: ext_xdp_user_utils.c ext_xdp_user_utils.h
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: clean

clean:
	rm -f *.o