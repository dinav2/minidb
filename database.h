#pragma once

#include "pager.h"
#include "table.h"

typedef struct {
  Pager pager;
} Database;

int db_open(Database *db, const char *filename);
int db_create_table(Database *db, u8 *table_name, Column *schema,
                    u32 column_count);
void db_close(Database *db);
