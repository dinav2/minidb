#include "unity.h"

#include "database.h"
#include "unity_internals.h"
#include <stdio.h>

Database db;
b8 db_opened = 0;
const char *test_db_path = "/tmp/minidb-test.db";

void setUp() {
  remove(test_db_path);

  TEST_ASSERT_EQUAL_INT(0, db_open(&db, test_db_path));
  db_opened = 1;
}

void tearDown() {
  if (db_opened) {
    db_close(&db);
    db_opened = 0;
  }

  remove(test_db_path);
}

extern void test_db_scan_next_empty_table_returns_scan_end(void);
extern void test_db_create_table_rejects_duplicates(void);
extern void test_db_create_table_rejects_long_name(void);
extern void test_db_scan_next_returns_inserted_row_after_reopen(void);
extern void test_db_insert_row_rejects_wrong_row_size(void);
extern void test_db_scan_table_rejects_nonexistent_table(void);

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_db_scan_next_empty_table_returns_scan_end);
  RUN_TEST(test_db_create_table_rejects_duplicates);
  RUN_TEST(test_db_create_table_rejects_long_name);
  RUN_TEST(test_db_scan_next_returns_inserted_row_after_reopen);
  RUN_TEST(test_db_insert_row_rejects_wrong_row_size);
  RUN_TEST(test_db_scan_table_rejects_nonexistent_table);

  return UNITY_END();
}
