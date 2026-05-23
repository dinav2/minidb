#include "table.h"
#include "page.h"

#include "types.h"
#include <string.h>

int table_create(char *table_name, Column *schema, u32 column_count,
                 Page *catalog_page, Page *schema_page) {
  CatalogRecord record = {.schema_page = get_page_id(schema_page)};
  if (strlen(table_name) >= sizeof(record.table_name)) {
    return 1;
  }

  memcpy(record.table_name, table_name, strlen(table_name) + 1);

  for (u32 i = 0; i < column_count; i++) {
    record.row_size += (schema + i)->size;
    page_add_record(schema_page, schema + i, sizeof(Column));
  }

  page_add_record(catalog_page, &record, sizeof(record));

  return 0;
}
