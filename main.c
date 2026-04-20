#include <stdio.h>

#include "database.h"
#include "pager.h"
#include "table.h"
#include "types.h"

int main(void) {
  Database db;
  db_open(&db, "test.db");
  printf("Number of pages: %u\n", db.pager.page_count);

  Column c1 = {.type = 0, .size = 4, .name = "column1"};
  db_create_table(&db, "Table1", &c1, 1);

  i32 buf = 156;
  for (u32 i = 0; i < 10000; i++) {
    db_insert_row(&db, "Table1", &buf, sizeof(i32));
  }

  db_close(&db);
  return 0;
}
