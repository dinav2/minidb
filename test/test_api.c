#include "database.h"
#include "table.h"
#include "unity.h"

extern Database db;
extern b8 db_opened;
extern const char *test_db_path;

void test_db_scan_next_empty_table_returns_scan_end() {
  char *table_name = "Table1";

  Column column1 = {.type = 0, .size = 4, .name = "column1"};
  int table_create = db_create_table(&db, table_name, &column1, 1);
  TEST_ASSERT_EQUAL_INT(0, table_create);

  Cursor cursor;
  int scan = db_scan_table(&db, table_name, &cursor);
  TEST_ASSERT_EQUAL_INT(0, scan);

  u32 buf;
  int result = db_scan_next(&db, &cursor, &buf);
  TEST_ASSERT_EQUAL_INT(SCAN_END, result);
}

void test_db_create_table_rejects_duplicates() {
  char *table_name = "Table1";

  Column column1 = {.type = 0, .size = 4, .name = "column1"};
  int table_create1 = db_create_table(&db, table_name, &column1, 1);
  TEST_ASSERT_EQUAL_INT(0, table_create1);

  int table_create2 = db_create_table(&db, table_name, &column1, 1);
  TEST_ASSERT_EQUAL_INT(1, table_create2);
}

void test_db_create_table_rejects_long_name() {
  char *table_name = "aaaabbbbccccddddeeeeffffgggghhhh";

  Column column1 = {.type = 0, .size = 4, .name = "column1"};
  int table_create = db_create_table(&db, table_name, &column1, 1);
  TEST_ASSERT_EQUAL_INT(1, table_create);
}

void test_db_scan_next_returns_inserted_row_after_reopen() {
  char *table_name = "Table1";

  Column column1 = {.type = 0, .size = 4, .name = "column1"};
  int table_create = db_create_table(&db, table_name, &column1, 1);
  TEST_ASSERT_EQUAL_INT(0, table_create);

  u32 buf = 42;
  int insert_row = db_insert_row(&db, table_name, &buf, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, insert_row);

  db_close(&db);
  db_opened = 0;

  int db_opened_check = db_open(&db, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, db_opened_check);
  db_opened = 1;

  Cursor cursor;
  int scan = db_scan_table(&db, table_name, &cursor);
  TEST_ASSERT_EQUAL_INT(0, scan);

  u32 result_buf;
  int result = db_scan_next(&db, &cursor, &result_buf);

  TEST_ASSERT_EQUAL_INT(SCAN_OK, result);
  TEST_ASSERT_EQUAL_MEMORY(&buf, &result_buf, sizeof(buf));
}
