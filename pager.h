#pragma once

#include <stdio.h>

#include "page.h"
#include "types.h"

#define POOL_SIZE 16

typedef struct BufferSlot {
  Page page;
  u32 page_id;
  b8 occupied;
  b8 dirty;
  struct BufferSlot *next;
  struct BufferSlot *prev;
} BufferSlot;

typedef struct {
  FILE *fp;
  BufferSlot pool[POOL_SIZE];
  BufferSlot *head;
  BufferSlot *tail;
  u32 page_count;
} Pager;

int cache_flush(Pager *pager);

int pager_open(Pager *pp, const char *filename);

const Page *pager_get_page(Pager *pager, u32 page_id);
Page *pager_get_page_for_write(Pager *pager, u32 page_id);

Page *pager_create_page(Pager *pp, u32 *page_id);
void pager_close(Pager *pp);
