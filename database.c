#include <stdio.h>
#include <string.h>

#include "database.h"
#include "page.h"
#include "pager.h"
#include "table.h"

int db_open(Database *db, const char *filename) {
  if (pager_open(&db->pager, filename) != 0) {
    fprintf(stderr, "error at pager_open()\n");
    return 1;
  }

  if (db->pager.page_count == 0) {
    // new database, create header and catalog pages
    u32 header_page_id, catalog_page_id;
    Page *header, *catalog;
    header = pager_create_page(&db->pager, &header_page_id);
    catalog = pager_create_page(&db->pager, &catalog_page_id);

    header_page_init(header, header_page_id);
    page_init(catalog, catalog_page_id, PAGE_TYPE_CATALOG);
  }

  return 0;
}

static int _db_find_table(Database *db, const Page *catalog, char *table_name,
                          CatalogRecord **record) {
  const Page *curr_catalog_page = catalog;

  while (curr_catalog_page != NULL) {
    u32 offset = PAGE_HEADER_SIZE;

    for (u32 i = 0; i < get_page_records(curr_catalog_page); i++) {
      CatalogRecord *r = (CatalogRecord *)(curr_catalog_page->data + offset);
      if (strcmp(r->table_name, table_name) == 0) {
        if (record != NULL) {
          pager_get_page_for_write(&db->pager, get_page_id(curr_catalog_page));
          *record = r;
        }
        return 0;
      }
      offset += sizeof(CatalogRecord);
    }

    u32 next_page_id = get_page_next_id(curr_catalog_page);
    if (next_page_id == 0) {
      break;
    }

    curr_catalog_page = pager_get_page(&db->pager, next_page_id);
  }

  return 1;
}

static Page *_db_find_free_catalog_page(Database *db, const Page *catalog) {
  const Page *curr_catalog_page = catalog;

  while (curr_catalog_page != NULL) {
    if (get_page_free(curr_catalog_page) < sizeof(CatalogRecord)) {

      u32 next_page_id = get_page_next_id(curr_catalog_page);
      if (next_page_id == 0) {
        break;
      }
      curr_catalog_page = pager_get_page(&db->pager, next_page_id);
    } else {
      Page *writable_catalog_page =
          pager_get_page_for_write(&db->pager, get_page_id(curr_catalog_page));
      return writable_catalog_page;
    }
  }

  u32 new_page_id;
  Page *new_page = pager_create_page(&db->pager, &new_page_id);
  if (!new_page) {
    return NULL;
  }
  page_init(new_page, new_page_id, PAGE_TYPE_CATALOG);

  Page *writable_catalog_page =
      pager_get_page_for_write(&db->pager, get_page_id(curr_catalog_page));
  set_page_next_id(writable_catalog_page, new_page_id);

  return new_page;
}

int db_create_table(Database *db, char *table_name, Column *schema,
                    u32 column_count) {
  if (strlen(table_name) >= 32) {
    fprintf(stderr, "table name is too long\n");
    return 1;
  }

  const Page *catalog = pager_get_page(&db->pager, 1);
  if (!catalog) {
    return 1;
  }

  if (_db_find_table(db, catalog, table_name, NULL) == 0) {
    fprintf(stderr, "table already exists\n");
    return 1;
  }

  Page *free_catalog_page = _db_find_free_catalog_page(db, catalog);
  if (!free_catalog_page) {
    return 1;
  }

  u32 schema_page_id;
  Page *schema_page = pager_create_page(&db->pager, &schema_page_id);
  if (!schema_page) {
    return 1;
  }

  page_init(schema_page, schema_page_id, PAGE_TYPE_SCHEMA);

  table_create(table_name, schema, column_count, free_catalog_page,
               schema_page);

  return 0;
}

int db_insert_row(Database *db, char *table_name, const void *buf, u32 length) {
  const Page *catalog = pager_get_page(&db->pager, 1);

  if (!catalog) {
    return 1;
  }

  CatalogRecord *table_record;
  if (_db_find_table(db, catalog, table_name, &table_record) != 0) {
    fprintf(stderr, "table does not exist");
    return 1;
  }

  if (table_record->row_size != length) {
    fprintf(stderr, "record is different than row size of table");
    return 1;
  }

  Page *data_page;

  if (table_record->page_end != 0) {
    data_page = pager_get_page_for_write(&db->pager, table_record->page_end);
    if (!data_page) {
      return 1;
    }

    if (page_can_fit_record(data_page, length)) {
      if (page_add_record(data_page, buf, length) != 0) {
        return 1;
      }

      table_record->record_count++;
      return 0;
    }
  }

  // Create new data page
  u32 data_page_id;
  Page *new_page = pager_create_page(&db->pager, &data_page_id);
  if (!new_page) {
    return 1;
  }

  page_init(new_page, data_page_id, PAGE_TYPE_DATA);

  if (page_add_record(new_page, buf, length) != 0) {
    return 1;
  }

  if (table_record->page_start == 0) {
    table_record->page_start = data_page_id;
  }

  if (table_record->page_end != 0) {
    Page *prev = pager_get_page_for_write(&db->pager, table_record->page_end);
    if (!prev) {
      return 1;
    }
    set_page_next_id(prev, data_page_id);
  }

  table_record->page_end = data_page_id;
  table_record->record_count++;

  return 0;
}

int db_delete_row(Database *db, Cursor *cursor) {
  Page *page = pager_get_page_for_write(&db->pager, cursor->curr_page_id);
  if (!page) {
    return 1;
  }

  page_delete_row(page, cursor->read_records, cursor->row_size);
  return 0;
}

int db_update_row(Database *db, Cursor *cursor, void *buf) {
  Page *page = pager_get_page_for_write(&db->pager, cursor->curr_page_id);
  if (!page) {
    return 1;
  }

  page_update_row(page, cursor->read_records, cursor->row_size, buf);
  return 0;
}

int db_scan_table(Database *db, char *table_name, Cursor *cursor) {
  const Page *catalog = pager_get_page(&db->pager, 1);
  if (!catalog) {
    return 1;
  }

  CatalogRecord *table_record;
  if (_db_find_table(db, catalog, table_name, &table_record) != 0) {
    fprintf(stderr, "table does not exist\n");
    return 1;
  }

  if (table_record->page_start == 0) {
    Cursor c = {.curr_page = NULL,
                .curr_page_id = 0,
                .read_records = 0,
                .row_size = table_record->row_size};
    *cursor = c;
    return 0;
  }

  const Page *first_page = pager_get_page(&db->pager, table_record->page_start);
  if (!first_page) {
    return 1;
  }

  Cursor c = {.curr_page = first_page,
              .curr_page_id = table_record->page_start,
              .read_records = 0,
              .row_size = table_record->row_size};

  *cursor = c;

  return 0;
}

int db_scan_next(Database *db, Cursor *cursor, void *buf) {
  if (cursor->curr_page_id == 0 || cursor->curr_page == NULL) {
    return SCAN_END;
  }

  u32 slot_size = cursor->row_size + RECORD_HEADER_SIZE;
  u32 offset = PAGE_HEADER_SIZE + cursor->read_records * slot_size;

  for (;;) {
    if (cursor->read_records >= get_page_records(cursor->curr_page)) {
      u32 next_page_id = get_page_next_id(cursor->curr_page);

      if (next_page_id == 0) {
        return SCAN_END;
      }

      cursor->read_records = 0;

      const Page *next_page = pager_get_page(&db->pager, next_page_id);

      cursor->curr_page = next_page;
      cursor->curr_page_id = next_page_id;
    }

    offset = PAGE_HEADER_SIZE + cursor->read_records * slot_size;

    if (cursor->curr_page->data[offset] == 0) {
      break;
    }

    cursor->read_records++;
  }

  memcpy(buf, cursor->curr_page->data + offset + RECORD_HEADER_SIZE,
         cursor->row_size);
  cursor->read_records++;

  return SCAN_OK;
}
void db_close(Database *db) { pager_close(&db->pager); }
