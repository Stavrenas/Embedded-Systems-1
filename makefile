CC = gcc
CFLAGS = -lpthread -lm

default: ex1

ex1: ex1.c utilities.c
	$(CC) -o $@ $^ $(CFLAGS)