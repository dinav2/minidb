#include <errno.h>
#include <stdio.h>

#include "types.h"

#include "page.h"
#include "pager.h"

static int _cache_init(Pager *pager) {
  for (u32 i = 0; i < POOL_SIZE; i++) {
    pager->pool[i].occupied = 0;
    pager->pool[i].dirty = 0;
    pager->pool[i].prev = NULL;
    pager->pool[i].next = NULL;
  }
  pager->head = NULL;
  pager->tail = NULL;

  return 0;
}

static BufferSlot *_cache_evict_slot(Pager *pager, BufferSlot *slot) {
  if (slot->dirty) {
    pager_write_page(pager, slot->page_id, slot->page.data);
  }

  slot->occupied = 0;
  slot->dirty = 0;

  if (slot == pager->tail && slot == pager->head) {
    pager->head = NULL;
    pager->tail = NULL;
    return slot;
  }

  if (slot == pager->tail) {
    pager->tail = pager->tail->prev;
  }

  if (slot->prev) {
    slot->prev->next = slot->next;
  }

  if (slot->next) {
    slot->next->prev = slot->prev;
  }

  slot->next = NULL;
  slot->prev = NULL;

  return slot;
}

static BufferSlot *_cache_lookup_slot(Pager *pager, u32 page_id) {
  for (u32 i = 0; i < POOL_SIZE; i++) {
    BufferSlot *slot = &pager->pool[i];
    if (slot->page_id == page_id && slot->occupied == 1) {
      return slot;
    }
  }

  return NULL;
}

static BufferSlot *_cache_reserve_slot(Pager *pager) {
  for (u32 i = 0; i < POOL_SIZE; i++) {
    if (!pager->pool[i].occupied) {
      return &pager->pool[i];
    }
  }

  // Pool is full, eviction is needed
  if (!pager->tail) {
    return NULL;
  }

  return _cache_evict_slot(pager, pager->tail);
}

static BufferSlot *_cache_add_slot(Pager *pager, u32 page_id, u8 dirty,
                                   u8 *is_new) {
  BufferSlot *tmp = _cache_lookup_slot(pager, page_id);

  if (!tmp) {
    for (u32 i = 0; i < POOL_SIZE; i++) {
      if (!pager->pool[i].occupied) {
        tmp = &pager->pool[i];
        break;
      }
    }

    if (tmp == NULL) {
      tmp = _cache_evict_slot(pager, pager->tail);
    }

    tmp->occupied = 1;
    tmp->page_id = page_id;
    if (is_new) {
      *is_new = 1;
    }
  }

  if (dirty) {
    tmp->dirty = 1;
  }

  if (tmp == pager->head) {
    return tmp;
  }

  if (!pager->head && !pager->tail) {
    pager->head = tmp;
    pager->tail = tmp;
    return tmp;
  }

  if (tmp->prev) {
    tmp->prev->next = tmp->next;
  }

  pager->head->prev = tmp;

  if (tmp == pager->tail) {
    pager->tail = tmp->prev;
  }

  tmp->prev = NULL;
  tmp->next = pager->head;

  pager->head = tmp;

  return tmp;
}

int cache_flush(Pager *pager) {
  for (u32 i = 0; i < POOL_SIZE; i++) {
    BufferSlot *slot = &pager->pool[i];
    if (slot->dirty && slot->occupied) {
      pager_write_page(pager, slot->page_id, slot->page.data);
      slot->dirty = 0;
    }
  }

  return 0;
}

int pager_open(Pager *pp, const char *filename) {
  pp->fp = fopen(filename, "r+b");
  if (!pp->fp) {
    if (errno != ENOENT) {
      fprintf(stderr, "error when opening file: %d\n", errno);
      return 1;
    }

    pp->fp = fopen(filename, "w+b");
    if (!pp->fp) {
      fprintf(stderr, "error when opening file: %d\n", errno);
      return 1;
    }
  }

  if (fseek(pp->fp, 0, SEEK_END) != 0) {
    fprintf(stderr, "fseek failed\n");
    return 1;
  }

  long pos = ftell(pp->fp);
  if (pos == -1L) {
    fprintf(stderr, "ftell failed\n");
    return 1;
  }

  pp->page_count = pos / PAGE_SIZE;

  if (pos != pp->page_count * PAGE_SIZE) {
    fprintf(stderr, "File may be corrupted\n");
    return 1;
  }

  _cache_init(pp);

  return 0;
}

// int pager_read_page(Pager *pp, u32 page, void *buf) {
//   if (page >= pp->page_count) {
//     return 1;
//   }
//
//   u8 is_new = 0;
//   BufferSlot *cache_slot = _cache_add_slot(pp, page, 0, &is_new);
//
//   if (!is_new) {
//     memcpy(buf, cache_slot->page.data, PAGE_SIZE);
//     return 0;
//   }
//
//   if (fseek(pp->fp, page * PAGE_SIZE, SEEK_SET) != 0) {
//     fprintf(stderr, "fseek() failed\n");
//     return 1;
//   }
//
//   usize n = fread(buf, 1, PAGE_SIZE, pp->fp);
//
//   if (n != PAGE_SIZE) {
//     fprintf(stderr, "fread failed\n");
//     return 1;
//   }
//
//   memcpy(cache_slot->page.data, buf, PAGE_SIZE);
//
//   return 0;
// }
//

const Page *pager_get_page(Pager *pager, u32 page_id) {
  if (page_id >= pager->page_count) {
    return NULL;
  }

  u8 is_new = 0;
  BufferSlot *cache_slot = _cache_add_slot(pager, page_id, 0, &is_new);

  if (!is_new) {
    return &cache_slot->page;
  }

  if (fseek(pager->fp, page_id * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek() failed\n");
    return NULL;
  }

  usize n = fread(cache_slot->page.data, 1, PAGE_SIZE, pager->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "fread failed\n");
    return NULL;
  }

  return &cache_slot->page;
}

Page *pager_get_page_for_write(Pager *pager, u32 page_id) {
  if (page_id >= pager->page_count) {
    return NULL;
  }

  u8 is_new = 0;
  BufferSlot *cache_slot = _cache_add_slot(pager, page_id, 1, &is_new);

  if (!is_new) {
    return &cache_slot->page;
  }

  if (fseek(pager->fp, page_id * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek() failed\n");
    return NULL;
  }

  usize n = fread(cache_slot->page.data, 1, PAGE_SIZE, pager->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "fread failed\n");
    return NULL;
  }

  return &cache_slot->page;
}

int pager_write_page(Pager *pp, u32 page, const void *buf) {
  if (page > pp->page_count) {
    return 1;
  }

  if (fseek(pp->fp, page * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek failed");
    return 1;
  }

  usize n = fwrite(buf, sizeof(u8), PAGE_SIZE, pp->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "error when writing into file");
    return 1;
  }

  if (fflush(pp->fp) == EOF) {
    fprintf(stderr, "error when flushing");
    return 1;
  }

  if (page == pp->page_count) {
    pp->page_count++;
  }

  return 0;
}

int pager_mark_dity(Pager *Pager, u32 page_id) { return 0; }

Page *pager_create_page(Pager *pp, u32 *page_id) {
  *page_id = pp->page_count;
  BufferSlot *slot = _cache_add_slot(pp, pp->page_count, 1, NULL);

  pp->page_count++;

  return &slot->page;
}

void pager_close(Pager *pp) {
  if (!pp->fp) {
    return;
  }

  cache_flush(pp);

  fclose(pp->fp);
}
