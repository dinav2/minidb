#pragma once

/* Page Layout (4096 bytes)
 *
 * +--------+--------------+------+
 * | Offset | Field        | Size |
 * +--------+--------------+------+
 * |      0 | page_id      |    4 |
 * |      4 | free_offset  |    4 |
 * |      8 | record_count |    4 |
 * |      12 | (reserved)   |  116 |
 * +--------+--------------+------+
 * |    128 | payload      | 3968 |
 * +--------+--------------+------+
 */

#define PAGE_SIZE 4096
#define PAGE_HEADER_SIZE 128

// Page Header
#define PAGE_ID_OFFSET 0
#define PAGE_FREE_OFFSET 4
#define PAGE_RECORD_COUNT_OFFSET 8
#define PAGE_TYPE_OFFSET 12

// Database Header

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

// temporal record definition
typedef struct {
  int id;
  int x;
  int y;
} Record;

u32 get_page_id(const Page *pp);
u32 get_page_free(const Page *pp);
u32 get_page_records(const Page *pp);

int set_page_next_id(Page *pp, u32 next_id);

int page_add_record(Page *pp, const void *record, u32 length);
int page_init(Page *pp, u32 page_count, u8 page_type);
int header_page_init(Page *pp, u32 page_id);
