#include <string.h>

#include "database.h"
#include "page.h"
#include "pager.h"
#include "table.h"

int db_open(Database *db, const char *filename) {
  pager_open(&db->pager, filename);

  if (db->pager.page_count == 0) {
    // new database, create header and catalog pages
    u32 header_page_id, catalog_page_id;
    pager_create_page(&db->pager, &header_page_id);
    pager_create_page(&db->pager, &catalog_page_id);

    Page header, catalog;
    header_page_init(&header, header_page_id);
    page_init(&catalog, catalog_page_id, PAGE_TYPE_CATALOG);

    pager_write_page(&db->pager, header_page_id, header.data);
    pager_write_page(&db->pager, catalog_page_id, catalog.data);
  }

  return 0;
}

static int _db_find_table(Page *catalog, u8 *table_name,
                          CatalogRecord *record) {
  u32 offset = PAGE_HEADER_SIZE;
  for (u32 i = 0; i < get_page_records(catalog); i++) {
    CatalogRecord *r = (CatalogRecord *)(catalog->data + offset);
    if (strcmp((char *)r->table_name, (char *)table_name) == 0) {
      if (record != NULL) {
        *record = *r;
      }
      return 0;
    }
    offset += sizeof(CatalogRecord);
  }

  return 1;
}

int db_create_table(Database *db, u8 *table_name, Column *schema,
                    u32 column_count) {
  Page catalog, schema_page;
  pager_read_page(&db->pager, 1, catalog.data);

  if (_db_find_table(&catalog, table_name, NULL) == 0) {
    fprintf(stderr, "table already exists");
    return 1;
  }

  if (get_page_free(&catalog) < sizeof(CatalogRecord)) {
    // create new catalog page
    return 1;
  }

  u32 schema_page_id;
  pager_create_page(&db->pager, &schema_page_id);
  page_init(&schema_page, schema_page_id, PAGE_TYPE_SCHEMA);

  create_table(table_name, schema, column_count, &catalog, &schema_page);

  pager_write_page(&db->pager, get_page_id(&catalog), catalog.data);
  pager_write_page(&db->pager, schema_page_id, schema_page.data);

  return 0;
}

void db_close(Database *db) { pager_close(&db->pager); }
