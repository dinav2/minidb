CC = gcc
CFLAGS = -Wall -pedantic -Wextra -g -std=c11

main : main.c page.c pager.c database.c table.c
	$(CC) $(CFLAGS) $^ -o $@
