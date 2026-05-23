CC = gcc
CFLAGS = -Wall -pedantic -Wextra -g -std=c11 -I. -Itest/vendor/unity

APP_SRC = main.c page.c pager.c database.c table.c
UNIT_TEST_SRC = test/unit_runner.c test/test_page.c test/test_pager.c test/vendor/unity/unity.c page.c pager.c database.c table.c
INTEGRATION_TEST_SRC = test/integration_runner.c test/test_api.c test/vendor/unity/unity.c page.c pager.c database.c table.c

.PHONY: test

main : $(APP_SRC)
	$(CC) $(CFLAGS) $^ -o $@

test-unit : $(UNIT_TEST_SRC)
	$(CC) $(CFLAGS) $^ -o unit_test_bin
	./unit_test_bin

test-integration : $(INTEGRATION_TEST_SRC)
	$(CC) $(CFLAGS) $^ -o integration_test_bin
	./integration_test_bin

test: test-unit test-integration
