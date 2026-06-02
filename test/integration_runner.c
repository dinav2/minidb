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
extern void test_db_create_table_creates_multipage_catalog(void);
extern void test_db_insert_row_finds_table_in_multipage_catalog(void);
extern void test_db_scan_table_scans_multipage_catalog(void);
extern void test_db_scan_next_cursor_read_pages_returns_correct_number(void);
extern void test_db_update_row_modifies_row(void);

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_db_scan_next_empty_table_returns_scan_end);
  RUN_TEST(test_db_create_table_rejects_duplicates);
  RUN_TEST(test_db_create_table_rejects_long_name);
  RUN_TEST(test_db_scan_next_returns_inserted_row_after_reopen);
  RUN_TEST(test_db_insert_row_rejects_wrong_row_size);
  RUN_TEST(test_db_scan_table_rejects_nonexistent_table);
  RUN_TEST(test_db_create_table_creates_multipage_catalog);
  RUN_TEST(test_db_insert_row_finds_table_in_multipage_catalog);
  RUN_TEST(test_db_scan_table_scans_multipage_catalog);
  RUN_TEST(test_db_scan_next_cursor_read_pages_returns_correct_number);
  RUN_TEST(test_db_update_row_modifies_row);

  return UNITY_END();
}
