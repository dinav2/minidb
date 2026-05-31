#include "page.h"
#include "pager.h"
#include "unity.h"

#include <stdio.h>
#include <string.h>

const char *test_db_path = "/tmp/minidb-test.db";

void test_pager_sets_page_count_to_zero() {
  Pager pager;

  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);
  TEST_ASSERT_EQUAL_UINT32(0, pager.page_count);
  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_create_page_increments_page_count() {
  Pager pager;

  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page = pager_create_page(&pager, &page_id);
  TEST_ASSERT_NOT_NULL(page);
  TEST_ASSERT_EQUAL_UINT32(0, page_id);
  TEST_ASSERT_EQUAL_UINT32(1, pager.page_count);

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_close_flushed_modified_page_bytes() {
  Pager pager;

  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page = pager_create_page(&pager, &page_id);
  TEST_ASSERT_NOT_NULL(page);
  TEST_ASSERT_EQUAL_UINT32(0, page_id);
  TEST_ASSERT_EQUAL_UINT32(1, pager.page_count);

  char data[PAGE_SIZE];
  memset(page->data, 0x10, sizeof(data));
  memset(data, 0x10, sizeof(data));

  pager_close(&pager);

  pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  const Page *fetched_page = pager_get_page(&pager, page_id);
  TEST_ASSERT_NOT_NULL(fetched_page);
  TEST_ASSERT_EQUAL_MEMORY(data, fetched_page->data, sizeof(data));

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_create_page_sets_page_dirty() {
  Pager pager;
  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page = pager_create_page(&pager, &page_id);
  TEST_ASSERT_NOT_NULL(page);

  BufferSlot *slot = NULL;
  for (u32 i = 0; i < POOL_SIZE; i++) {
    BufferSlot *tmp_slot = &pager.pool[i];
    if (tmp_slot->occupied == 1 && tmp_slot->page_id == page_id) {
      slot = tmp_slot;
      break;
    }
  }

  TEST_ASSERT_NOT_NULL(slot);

  TEST_ASSERT_EQUAL_INT8(1, slot->dirty);

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_cache_overflow_persists_evicted_pages() {
  Pager pager;

  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page;
  for (u32 i = 0; i < POOL_SIZE * 2; i++) {
    page = pager_create_page(&pager, &page_id);
    TEST_ASSERT_NOT_NULL(page);
    TEST_ASSERT_EQUAL_UINT32(i, page_id);
    TEST_ASSERT_EQUAL_UINT32(i + 1, pager.page_count);

    memset(page->data, i, sizeof(page->data));
  }

  pager_close(&pager);

  pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  const Page *fetched_page;
  for (u32 i = 0; i < POOL_SIZE * 2; i++) {
    fetched_page = pager_get_page(&pager, i);
    TEST_ASSERT_NOT_NULL(fetched_page);

    char data[PAGE_SIZE];
    memset(data, i, sizeof(data));
    TEST_ASSERT_EQUAL_MEMORY(data, fetched_page->data, sizeof(data));
  }

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_create_page_assigns_sequential_ids() {
  Pager pager;

  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page;
  for (u32 i = 0; i < 100; i++) {
    page = pager_create_page(&pager, &page_id);
    TEST_ASSERT_NOT_NULL(page);
    TEST_ASSERT_EQUAL_UINT32(i, page_id);
    TEST_ASSERT_EQUAL_UINT32(i + 1, pager.page_count);
  }

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_page_count_preserved_after_reopen() {
  Pager pager;

  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page;
  for (u32 i = 0; i < 100; i++) {
    page = pager_create_page(&pager, &page_id);
    TEST_ASSERT_NOT_NULL(page);
    TEST_ASSERT_EQUAL_UINT32(i, page_id);
    TEST_ASSERT_EQUAL_UINT32(i + 1, pager.page_count);
  }

  pager_close(&pager);

  pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  TEST_ASSERT_EQUAL_UINT32(100, pager.page_count);

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_get_page_observes_new_page_updates_before_flush() {
  Pager pager;
  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page = pager_create_page(&pager, &page_id);
  TEST_ASSERT_NOT_NULL(page);

  memset(page->data, 0x17, sizeof(page->data));

  const Page *fetched_page = pager_get_page(&pager, page_id);
  TEST_ASSERT_NOT_NULL(fetched_page);

  char expected[PAGE_SIZE];
  memset(expected, 0x17, sizeof(expected));

  TEST_ASSERT_EQUAL_MEMORY(expected, fetched_page->data, sizeof(expected));

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_get_page_observes_updates_from_write_access() {
  Pager pager;
  remove(test_db_path);
  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page = pager_create_page(&pager, &page_id);
  TEST_ASSERT_NOT_NULL(page);

  memset(page->data, 0x17, sizeof(page->data));

  pager_close(&pager);

  pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  Page *page_for_write = pager_get_page_for_write(&pager, page_id);
  TEST_ASSERT_NOT_NULL(page_for_write);
  memset(page_for_write->data, 0x27, sizeof(page_for_write->data));

  const Page *fetched_page = pager_get_page(&pager, page_id);
  TEST_ASSERT_NOT_NULL(fetched_page);

  char expected[PAGE_SIZE];
  memset(expected, 0x27, sizeof(expected));

  TEST_ASSERT_EQUAL_MEMORY(expected, fetched_page->data, sizeof(expected));

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_get_page_returns_null_for_out_of_bounds_page_id() {
  Pager pager;
  remove(test_db_path);

  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id = 1;
  const Page *page = pager_get_page(&pager, page_id);
  TEST_ASSERT_NULL(page);

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_get_page_for_write_returns_null_for_out_of_bounds_page_id() {
  Pager pager;
  remove(test_db_path);

  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id = 1;
  const Page *page = pager_get_page_for_write(&pager, page_id);
  TEST_ASSERT_NULL(page);

  pager_close(&pager);
  remove(test_db_path);
}

void test_cache_flush_persists_dirty_page_without_closing_pager() {
  Pager pager;
  remove(test_db_path);

  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page = pager_create_page(&pager, &page_id);
  TEST_ASSERT_NOT_NULL(page);

  char expected[PAGE_SIZE];
  memset(page->data, 0x22, sizeof(page->data));
  memset(expected, 0x22, sizeof(expected));

  int flushed = cache_flush(&pager);
  TEST_ASSERT_EQUAL_INT(0, flushed);

  FILE *fp = fopen(test_db_path, "rb");
  TEST_ASSERT_NOT_NULL(fp);

  int seek = fseek(fp, PAGE_SIZE * page_id, SEEK_SET);
  TEST_ASSERT_EQUAL_INT(0, seek);

  char actual[PAGE_SIZE];
  size_t bytes = fread(actual, 1, PAGE_SIZE, fp);
  TEST_ASSERT_EQUAL_size_t(PAGE_SIZE, bytes);

  fclose(fp);

  TEST_ASSERT_EQUAL_MEMORY(expected, actual, sizeof(expected));

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_get_page_promotes_accessed_slot_to_cache_head() {
  Pager pager;
  remove(test_db_path);

  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page = NULL;
  for (u32 i = 0; i < POOL_SIZE; i++) {
    page = pager_create_page(&pager, &page_id);
    TEST_ASSERT_NOT_NULL(page);
    TEST_ASSERT_EQUAL_UINT32(i, page_id);
  }

  BufferSlot *slot0 = NULL;
  for (u32 i = 0; i < POOL_SIZE; i++) {
    BufferSlot *tmp_slot = &pager.pool[i];
    if (tmp_slot->occupied == 1 && tmp_slot->page_id == 0) {
      slot0 = tmp_slot;
      break;
    }
  }

  TEST_ASSERT_NOT_NULL(slot0);
  TEST_ASSERT_EQUAL_PTR(pager.tail, slot0);

  const Page *page0 = pager_get_page(&pager, 0);
  TEST_ASSERT_NOT_NULL(page0);
  TEST_ASSERT_EQUAL_PTR(pager.head, slot0);

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_create_page_evicts_lru_page_on_full_cache() {
  Pager pager;
  remove(test_db_path);

  int pager_opened = pager_open(&pager, test_db_path);
  TEST_ASSERT_EQUAL_INT(0, pager_opened);

  u32 page_id;
  Page *page;
  for (u32 i = 0; i < POOL_SIZE; i++) {
    page = pager_create_page(&pager, &page_id);
    TEST_ASSERT_NOT_NULL(page);
  }

  BufferSlot *slot0 = NULL;
  for (u32 i = 0; i < POOL_SIZE; i++) {
    BufferSlot *tmp_slot = &pager.pool[i];
    if (tmp_slot->occupied == 1 && tmp_slot->page_id == 0) {
      slot0 = tmp_slot;
      break;
    }
  }

  TEST_ASSERT_NOT_NULL(slot0);
  TEST_ASSERT_EQUAL_PTR(pager.tail, slot0);

  page = pager_create_page(&pager, &page_id);
  TEST_ASSERT_NOT_NULL(page);

  b8 found_page_0 = 0;
  b8 found_new_page = 0;
  for (u32 i = 0; i < POOL_SIZE; i++) {
    BufferSlot *slot = &pager.pool[i];
    if (!slot->occupied) {
      continue;
    }

    if (slot->page_id == page_id) {
      found_new_page = 1;
    }

    if (slot->page_id == 0) {
      found_page_0 = 1;
    }
  }

  TEST_ASSERT_FALSE(found_page_0);
  TEST_ASSERT_TRUE(found_new_page);

  pager_close(&pager);
  remove(test_db_path);
}
