CC = gcc
OBJS = seker_client.o seker_helpers.o
EXEC = seker_client
COMP_FLAG = -Wall -g

all: $(EXEC)
$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $@
seker_client.o: seker_client.c seker_client.h seker_helpers.h
	$(CC) $(COMP_FLAG) -c $*.c
clean:
	rm -f $(OBJS) $(EXEC)
