#include "page.h"
#include "unity.h"

void test_page_init_sets_expected_header(void) {
  Page page;

  page_init(&page, 7, PAGE_TYPE_DATA);

  TEST_ASSERT_EQUAL_UINT32(7, get_page_id(&page));
  TEST_ASSERT_EQUAL_UINT8(PAGE_TYPE_DATA, get_page_type(&page));
  TEST_ASSERT_EQUAL_UINT32(0, get_page_records(&page));
  TEST_ASSERT_EQUAL_UINT32(PAGE_SIZE - PAGE_HEADER_SIZE, get_page_free(&page));
  TEST_ASSERT_EQUAL_UINT32(0, get_page_next_id(&page));
}

void test_page_add_record_updates_metadata_and_writes_payload() {
  Page page;

  page_init(&page, 7, PAGE_TYPE_DATA);

  i32 buf = 123;
  int result = page_add_record(&page, &buf, sizeof(buf));

  TEST_ASSERT_EQUAL_INT(0, result);
  TEST_ASSERT_EQUAL_UINT32(1, get_page_records(&page));
  TEST_ASSERT_EQUAL_UINT32(PAGE_SIZE - PAGE_HEADER_SIZE - RECORD_HEADER_SIZE -
                               sizeof(buf),
                           get_page_free(&page));

  TEST_ASSERT_EQUAL_MEMORY(
      &buf, (page.data + PAGE_HEADER_SIZE + RECORD_HEADER_SIZE), sizeof(buf));
}

void test_page_add_record_fails_when_page_has_no_space() {
  Page page;

  page_init(&page, 7, PAGE_TYPE_DATA);

  i32 buf = 123;
  u32 size =
      (PAGE_SIZE - PAGE_HEADER_SIZE) / (sizeof(buf) + RECORD_HEADER_SIZE);
  int add_record = 0;
  for (u32 i = 0; i < size; i++) {
    add_record = page_add_record(&page, &buf, sizeof(buf));
    TEST_ASSERT_EQUAL_INT(0, add_record);
  }
  TEST_ASSERT_EQUAL_UINT32(size, get_page_records(&page));

  add_record = page_add_record(&page, &buf, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(1, add_record);

  TEST_ASSERT_EQUAL_UINT32(size, get_page_records(&page));
}

void test_page_delete_row_marks_tombstone_byte() {
  TEST_IGNORE();
  Page page;

  page_init(&page, 7, PAGE_TYPE_DATA);

  i32 buf = 123;
  int add_record = page_add_record(&page, &buf, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, add_record);

  int delete_row = page_delete_row(&page, 0, sizeof(buf));
  TEST_ASSERT_EQUAL_INT(0, delete_row);
}
