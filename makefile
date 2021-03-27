CC = gcc
CFLAGS = -lpthread -lm -g

default: ex1

ex1: ex1.c utilities.c
	$(CC) -o $@ $^ $(CFLAGS)