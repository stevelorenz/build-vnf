#
# Makefile
#

CFLAGS=-g -Wall
LDFLAGS=-lpthread
OBJECTS=l4_udp_nc.o

all: l4_udp_nc.out

l4_udp_nc.out: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

.PHONY: clean

clean:
	-rm -f $(OBJECTS) l4_udp_nc.out


# vim:ft=make
#
