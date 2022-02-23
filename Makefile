CC = gcc
CFLAGS = -I./include
INCLUDE = include/server-tools.h
OBJ_DIR = obj
SRC_DIR = src

SERVER_OBJ = server.o server-tools.o 
SERVER = $(patsubst %,$(OBJ_DIR)/%,$(SERVER_OBJ))
SERVER_EXECUTABLE = bin/server

CLIENT_OBJ = client.o server-tools.o 
CLIENT = $(patsubst %,$(OBJ_DIR)/%,$(CLIENT_OBJ))
CLIENT_EXECUTABLE = bin/client

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(INCLUDE)
	$(CC) -c -o $@ $< $(CFLAGS)

server: $(SERVER)
	$(CC) -o ${SERVER_EXECUTABLE} $^ $(CFLAGS)

client: $(CLIENT)
	$(CC) -o ${CLIENT_EXECUTABLE} $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f $(SERVER_EXECUTABLE) $(CLIENT_EXECUTABLE) $(BINARIES) *.o obj/*.o
