#pragma once

#include <stdio.h>

#include "types.h"

typedef struct {
  FILE *fp;
  u32 page_count;
} Pager;

int pager_open(Pager *pp, const char *filename);
int pager_read_page(Pager *pp, u32 page, void *buf);
int pager_write_page(Pager *pp, u32 page, const void *buf);
int pager_create_page(Pager *pp, u32 *page_id);
void pager_close(Pager *pp);
