#include <stdio.h>

#include "database.h"
#include "pager.h"
#include "table.h"
#include "types.h"

int main(void) {
  Database db;
  db_open(&db, "database.db");
  printf("Number of pages: %u\n", db.pager.page_count);

  Column c1 = {.type = 0, .size = 4, .name = "column1"};
  db_create_table(&db, "Table1", &c1, 1);

  i32 buf = 156;
  for (u32 i = 0; i < 10; i++) {
    db_insert_row(&db, "Table1", &buf, sizeof(i32));
    buf += 10;
  }

  Cursor c;
  db_scan_table(&db, "Table1", &c);

  i32 row;
  u32 count = 0;
  while (db_scan_next(&db, &c, &row) == SCAN_OK) {
    printf("%u\n", row);
    if (count % 2 == 0) {
      db_delete_row(&db, &c);
    }

    count++;
  }

  printf("\n\n");
  db_scan_table(&db, "Table1", &c);

  while (db_scan_next(&db, &c, &row) == SCAN_OK) {
    printf("%u\n", row);
  }

  db_close(&db);
  return 0;
}
