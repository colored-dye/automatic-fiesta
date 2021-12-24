CC = gcc
COMMON = ../myprot.h
CLIENT_SRC = $(COMMON) client.c
CFLAGS = -Wall -g -pthread

all: client

client: $(CLIENT_SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm client
