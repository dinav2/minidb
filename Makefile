CC = gcc
CFLAGS = -Wall -pedantic -Wextra -g -std=c11 -I.

APP_SRC = main.c page.c pager.c database.c table.c
TEST_SRC = tests/test.c page.c pager.c database.c table.c

main : $(APP_SRC)
	$(CC) $(CFLAGS) $^ -o $@

test : $(TEST_SRC)
	$(CC) $(CFLAGS) $^ -o $@
