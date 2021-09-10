CC = gcc
CFLAGS = -lpthread -lm

default: main

main: main.c utilities.c
	$(CC) -o $@ $^ $(CFLAGS)
