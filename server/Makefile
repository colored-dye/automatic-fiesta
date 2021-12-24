CC = gcc
COMMON = ../myprot.h
SERVER_SRC = $(COMMON) server.c
CFLAGS = -Wall -g -pthread

all: server

server: $(SERVER_SRC)
	$(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm server
