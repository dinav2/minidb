#include <string.h>

#include "page.h"

static PageHeader *page_header(Page *pp) { return (PageHeader *)pp->data; }
static DatabaseHeader *db_header(Page *pp) {
  return (DatabaseHeader *)(pp->data + PAGE_HEADER_SIZE);
}

static const PageHeader *page_header_const(const Page *pp) {
  return (PageHeader *)pp->data;
}

u32 get_page_id(const Page *pp) { return page_header_const(pp)->page_id; }

u8 get_page_type(const Page *pp) { return page_header_const(pp)->page_type; }

u32 get_page_records(const Page *pp) {
  return page_header_const(pp)->record_count;
}

u32 get_page_free(const Page *pp) {
  return PAGE_SIZE - page_header_const(pp)->free_offset;
}

u32 get_page_next_id(const Page *pp) {
  return page_header_const(pp)->next_page_id;
}

int set_page_next_id(Page *pp, u32 next_id) {
  page_header(pp)->next_page_id = next_id;

  return 0;
}

int page_init(Page *pp, u32 page_id, u8 page_type) {
  // Initialize buffer with all 0s
  memset(pp->data, 0, PAGE_SIZE);

  PageHeader *h = page_header(pp);

  // write page header
  h->page_id = page_id;
  h->free_offset = PAGE_HEADER_SIZE;
  h->page_type = page_type;

  return 0;
}

int header_page_init(Page *pp, u32 page_id) {
  page_init(pp, page_id, PAGE_TYPE_HEADER);
  DatabaseHeader *dh = db_header(pp);

  dh->hdr_page_size = PAGE_SIZE;
  dh->hdr_version = 1;

  return 0;
}

int page_add_record(Page *pp, const void *record, u32 length) {
  PageHeader *header = page_header(pp);
  u32 free_bytes = PAGE_SIZE - header->free_offset;
  u32 record_header =
      (header->page_type == PAGE_TYPE_DATA) ? RECORD_HEADER_SIZE : 0;

  if (length + record_header > free_bytes) {
    return 1;
  }

  // Leave space for row header
  memset(pp->data + header->free_offset, 0, record_header);

  memcpy(pp->data + header->free_offset + record_header, record, length);
  header->free_offset += length + record_header;
  header->record_count++;

  return 0;
}

b8 page_can_fit_record(const Page *pp, u32 length) {
  const PageHeader *header = page_header_const(pp);
  u32 record_header =
      (header->page_type == PAGE_TYPE_DATA) ? RECORD_HEADER_SIZE : 0;

  return length <= (PAGE_SIZE - header->free_offset) - record_header;
}

int page_delete_row(Page *pp, u32 record_index, u16 row_size) {
  memset(pp->data + PAGE_HEADER_SIZE +
             (record_index - 1) * (row_size + RECORD_HEADER_SIZE),
         0x01, 1);

  return 0;
}
