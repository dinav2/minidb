#include <errno.h>
#include <stdio.h>

#include "types.h"

#include "page.h"
#include "pager.h"

static int pager_write_page(Pager *pp, u32 page, const void *buf);

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

static void _cache_mark_dirty(BufferSlot *slot) { slot->dirty = 1; }

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

  if (!pager->tail) {
    return NULL;
  }

  return _cache_evict_slot(pager, pager->tail);
}

static int _cache_load_slot(Pager *pager, BufferSlot *slot, u32 page_id) {

  if (fseek(pager->fp, page_id * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek() failed\n");
    return 1;
  }

  usize n = fread(slot->page.data, 1, PAGE_SIZE, pager->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "fread failed\n");
    return 1;
  }

  slot->page_id = page_id;
  slot->occupied = 1;
  slot->dirty = 0;
  slot->next = NULL;
  slot->prev = NULL;

  return 0;
}

static int _cache_init_new_slot(BufferSlot *slot, u32 page_id) {
  slot->page_id = page_id;
  slot->occupied = 1;
  slot->dirty = 0;
  slot->next = NULL;
  slot->prev = NULL;

  return 0;
}

static int _cache_promote_slot(Pager *pager, BufferSlot *slot) {
  if (!slot) {
    return -1;
  }

  if (slot == pager->head) {
    return 0;
  }

  // cache is empty
  if (!pager->head && !pager->tail) {
    pager->head = slot;
    pager->tail = slot;
    slot->prev = NULL;
    slot->next = NULL;
  } else {
    // existing slot
    if (slot->prev) {
      slot->prev->next = slot->next;
      if (slot->next) {
        slot->next->prev = slot->prev;
      } else {
        pager->tail = slot->prev;
      }
    }

    slot->prev = NULL;
    slot->next = pager->head;

    pager->head->prev = slot;
    pager->head = slot;
  }

  return 0;
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

const Page *pager_get_page(Pager *pager, u32 page_id) {
  if (page_id >= pager->page_count) {
    return NULL;
  }

  BufferSlot *slot;
  slot = _cache_lookup_slot(pager, page_id);

  if (slot) {
    _cache_promote_slot(pager, slot);
    return &slot->page;
  }

  slot = _cache_reserve_slot(pager);

  if (slot) {
    if (_cache_load_slot(pager, slot, page_id) != 0) {
      fprintf(stderr, "cache load slot failed\n");
      return NULL;
    }
    _cache_promote_slot(pager, slot);
    return &slot->page;
  }

  return NULL;
}

Page *pager_get_page_for_write(Pager *pager, u32 page_id) {
  if (page_id >= pager->page_count) {
    return NULL;
  }

  BufferSlot *slot;
  slot = _cache_lookup_slot(pager, page_id);

  if (slot) {
    _cache_mark_dirty(slot);
    _cache_promote_slot(pager, slot);
    return &slot->page;
  }

  slot = _cache_reserve_slot(pager);

  if (slot) {
    if (_cache_load_slot(pager, slot, page_id) != 0) {
      fprintf(stderr, "cache load slot failed\n");
      return NULL;
    }
    _cache_mark_dirty(slot);
    _cache_promote_slot(pager, slot);
    return &slot->page;
  }

  return NULL;
}

static int pager_write_page(Pager *pp, u32 page, const void *buf) {
  if (page > pp->page_count) {
    return 1;
  }

  if (fseek(pp->fp, page * PAGE_SIZE, SEEK_SET) != 0) {
    fprintf(stderr, "fseek failed\n");
    return 1;
  }

  usize n = fwrite(buf, sizeof(u8), PAGE_SIZE, pp->fp);

  if (n != PAGE_SIZE) {
    fprintf(stderr, "error when writing into file\n");
    return 1;
  }

  if (fflush(pp->fp) == EOF) {
    fprintf(stderr, "error when flushing\n");
    return 1;
  }

  if (page == pp->page_count) {
    pp->page_count++;
  }

  return 0;
}

Page *pager_create_page(Pager *pp, u32 *page_id) {
  *page_id = pp->page_count;
  BufferSlot *slot = _cache_reserve_slot(pp);
  if (!slot) {
    fprintf(stderr, "error when reserving slot\n");
    return NULL;
  }
  _cache_init_new_slot(slot, *page_id);
  _cache_mark_dirty(slot);
  _cache_promote_slot(pp, slot);

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
