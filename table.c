#include "table.h"
#include "page.h"

#include "types.h"
#include <string.h>

int create_table(u8 *table_name, Column *schema, u32 column_count,
                 Page *catalog_page, Page *schema_page) {
  CatalogRecord record = {.schema_page = get_page_id(schema_page)};
  strncpy((char *)record.table_name, (char *)table_name, 31);

  page_add_record(catalog_page, &record, sizeof(record));

  for (u32 i = 0; i < column_count; i++) {
    page_add_record(schema_page, schema + i, sizeof(Column));
  }

  return 0;
}
