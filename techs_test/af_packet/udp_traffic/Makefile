CFLAGS=-g -Wall
LDFLAGS=-lpthread
C_OBJECTS=client.o
S_OBJECTS=server.o

all: client.out server.out

client.out: $(C_OBJECTS)
	$(CC) -o $@ $(C_OBJECTS) $(LDFLAGS)

server.out: $(S_OBJECTS)
	$(CC) -o $@ $(S_OBJECTS)

.PHONY: clean

clean:
	-rm -f $(C_OBJECTS) client.out
	-rm -f $(S_OBJECTS) server.out
