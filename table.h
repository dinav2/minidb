#pragma once

#include "page.h"
#include "types.h"

typedef struct {
  u32 page_start;
  u32 page_end;
  u32 record_count;
  u32 schema_page;
  u8 table_name[32];
} CatalogRecord;

typedef struct {
  u16 type;
  u16 size;
  u8 name[16];
} Column;

int create_table(u8 *table_name, Column *schema, u32 column_count,
                 Page *catalog_page, Page *schema_page);
