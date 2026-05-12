#include <stdio.h>

#include "database.h"
#include "table.h"

static int test_create_page_persists() {
  Database db;
  b8 db_opened = 0;
  const char *path = "/tmp/minidb-test-persistent.db";
  remove(path);

  if (db_open(&db, path) != 0) {
    goto error_cleanup;
  }
  db_opened = 1;

  Column column1 = {.type = 0, .size = 4, .name = "column1"};
  if (db_create_table(&db, "Table1", &column1, 1) != 0) {
    goto error_cleanup;
  }

  i32 buf = 156;
  for (u32 i = 0; i < 900; i++) {
    if (db_insert_row(&db, "Table1", &buf, sizeof(i32)) != 0) {
      goto error_cleanup;
    }
    buf += 10;
  }

  db_close(&db);
  db_opened = 0;

  // Check if persisted
  Database db2;
  b8 db2_opened = 0;
  if (db_open(&db2, path) != 0) {
    goto error_cleanup;
  }
  db2_opened = 1;

  Cursor c;
  if (db_scan_table(&db2, "Table1", &c) != 0) {
    goto error_cleanup;
  }

  i32 test = 156;
  i32 num;
  for (u32 i = 0; i < 900; i++) {
    if (db_scan_next(&db2, &c, &num) != 0) {
      goto error_cleanup;
    }

    if (num != test) {
      goto error_cleanup;
    }

    test += 10;
  }

  if (db_scan_next(&db2, &c, &num) != SCAN_END) {
    goto error_cleanup;
  }

  db_close(&db2);
  remove(path);

  return 0;

error_cleanup:
  if (db_opened) {
    db_close(&db);
  }
  if (db2_opened) {
    db_close(&db2);
  }
  remove(path);

  return 1;
}

int main() {
  if (test_create_page_persists() != 0) {
    fprintf(stderr, "failure at test 1");
  } else {
    printf("Test 1 passed succesfully");
  }
  return 0;
}
