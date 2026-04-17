CC = gcc
CFLAGS = -Wall -pedantic -Wextra -g -std=c11

main : main.c
	$(CC) $(CFLAGS) $< -o $@
