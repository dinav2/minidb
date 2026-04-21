#pragma once

#include "pager.h"
#include "table.h"

#define SCAN_OK 0
#define SCAN_END 1
#define SCAN_ERROR 2

typedef struct {
  Pager pager;
} Database;

typedef struct {
  Page curr_page;
  u32 read_records;
  u16 row_size;
} Cursor;

int db_open(Database *db, const char *filename);
int db_create_table(Database *db, char *table_name, Column *schema,
                    u32 column_count);
int db_insert_row(Database *db, char *table_name, const void *buf, u32 length);
int db_delete_row(Database *db, Cursor *cursor);
int db_scan_table(Database *db, char *table_name, Cursor *cursor);
int db_scan_next(Database *db, Cursor *cursor, void *buf);
void db_close(Database *db);
