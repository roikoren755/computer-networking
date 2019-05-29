CC = gcc
CLIENT_OBJ = seker_client.o
SERVER_OBJ = seker_server.o
HELPERS_OBJ = seker_helpers.o
OBJS = $(CLIENT_OBJ) $(SERVER_OBJ) $(HELPERS_OBJ)
CLIENT_EXEC = seker_client
SERVER_EXEC = seker_server
EXEC = $(CLIENT_EXEC) $(SERVER_EXEC)
COMP_FLAG = -Wall -g

all: $(EXEC)
$(CLIENT_EXEC): $(CLIENT_OBJ) $(HELPERS_OBJ)
	$(CC) $(CLIENT_OBJ) $(HELPERS_OBJ) -o $@
$(SERVER_EXEC): $(SERVER_OBJ) $(HELPERS_OBJ)
	$(CC) $(SERVER_OBJ) $(HELPERS_OBJ) -o $@
seker_client.o: seker_client.c seker_helpers.h
	$(CC) $(COMP_FLAG) -c $*.c
seker_server.o: seker_server.c seker_helpers.h
	$(CC) $(COMP_FLAG) -c $*.c
seker_helpers.o: seker_helpers.c seker_helpers.h
	$(CC) $(COMP_FLAG) -c $*.c
clean:
	rm -f $(OBJS) $(EXEC)
