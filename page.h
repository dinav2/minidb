#pragma once

/* Page Layout (4096 bytes)
 *
 * +--------+--------------+------+
 * | Offset | Field        | Size |
 * +--------+--------------+------+
 * |      0 | page_id      |    4 |
 * |      4 | free_offset  |    4 |
 * |      8 | record_count |    4 |
 * |     12 | next_page_id |    4 |
 * |     16 | page_type    |    1 |
 * |     17 | (reserved)   |  111 |
 * +--------+--------------+------+
 * |    128 | payload      | 3968 |
 * +--------+--------------+------+
 */

#define PAGE_SIZE 4096
#define PAGE_HEADER_SIZE 128

// Record Header
#define RECORD_HEADER_SIZE 1

// Page Types
#define PAGE_TYPE_HEADER 0x01
#define PAGE_TYPE_DATA 0x02
#define PAGE_TYPE_CATALOG 0x03
#define PAGE_TYPE_SCHEMA 0x04

#include "types.h"

typedef struct {
  u16 hdr_version;
  u16 hdr_page_size;
} DatabaseHeader;

typedef struct {
  u32 page_id;
  u32 free_offset;
  u32 record_count;
  u32 next_page_id;
  u8 page_type;
} PageHeader;

typedef struct {
  u8 data[PAGE_SIZE];
} Page;

u32 get_page_id(const Page *pp);
u8 get_page_type(const Page *pp);
u32 get_page_free(const Page *pp);
u32 get_page_records(const Page *pp);
u32 get_page_next_id(const Page *pp);

int set_page_next_id(Page *pp, u32 next_id);

int page_add_record(Page *pp, const void *record, u32 length);
int page_update_row(Page *pp, u32 record_index, u16 row_size, void *buf);
int page_init(Page *pp, u32 page_count, u8 page_type);
b8 page_can_fit_record(const Page *pp, u32 length);
int page_delete_row(Page *pp, u32 record_index, u16 row_size);
int header_page_init(Page *pp, u32 page_id);
