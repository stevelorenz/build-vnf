CFLAGS=-g -Wall
LDFLAGS=-lpthread
C_OBJECTS=client.o
S_OBJECTS=server.o

all: client server

client: $(C_OBJECTS)
	$(CC) -o $@ $(C_OBJECTS) $(LDFLAGS)

server: $(S_OBJECTS)
	$(CC) -o $@ $(S_OBJECTS)

.PHONY: clean

clean:
	-rm -f $(C_OBJECTS) client
	-rm -f $(S_OBJECTS) server
