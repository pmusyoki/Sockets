CC=gcc
CFLAGS=-I.
DEPS = server-tools.h
SERVER = server.o server-tools.o 
CLIENT = client.o server-tools.o
BINARIES = server client

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(SERVER)
	$(CC) -o $@ $^ $(CFLAGS)

client: $(CLIENT)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(BINARIES) *.o
