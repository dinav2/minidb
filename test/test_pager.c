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

  const Page *page2 = pager_get_page(&pager, page_id);
  TEST_ASSERT_NOT_NULL(page2);
  TEST_ASSERT_EQUAL_MEMORY(data, page2->data, sizeof(data));

  pager_close(&pager);
  remove(test_db_path);
}

void test_pager_cache() {
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

  const Page *page2 = pager_get_page(&pager, page_id);
  TEST_ASSERT_NOT_NULL(page2);
  TEST_ASSERT_EQUAL_MEMORY(data, page2->data, sizeof(data));

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
