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

u32 get_page_records(const Page *pp) {
  return page_header_const(pp)->record_count;
}

u32 get_page_free(const Page *pp) {
  return PAGE_SIZE - page_header_const(pp)->free_offset;
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

  if (length > free_bytes) {
    // create/look for a new page
    return 1;
  } else {
    memcpy(pp->data + header->free_offset, record, length);
    header->free_offset += length;
  }

  header->record_count++;

  return 0;
}
