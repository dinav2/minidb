#pragma once

#include "pager.h"
#include "table.h"

typedef struct {
  Pager pager;
} Database;

int db_open(Database *db, const char *filename);
int db_create_table(Database *db, char *table_name, Column *schema,
                    u32 column_count);
int db_insert_row(Database *db, char *table_name, const void *buf, u32 length);
void db_close(Database *db);
