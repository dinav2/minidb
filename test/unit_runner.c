#include "unity.h"
#include "unity_internals.h"

void setUp() {}

void tearDown() {}

// Page
extern void test_page_init_sets_expected_header(void);
extern void test_page_add_record_updates_metadata_and_writes_payload(void);
extern void test_page_add_record_fails_when_page_has_no_space(void);
extern void test_page_delete_row_marks_tombstone_byte(void);

// Pager
extern void test_pager_sets_page_count_to_zero(void);
extern void test_pager_create_page_increments_page_count(void);
extern void test_pager_close_flushed_modified_page_bytes(void);
extern void test_pager_create_page_assigns_sequential_ids(void);
extern void test_pager_page_count_preserved_after_reopen(void);

int main(void) {
  UNITY_BEGIN();

  // Page
  RUN_TEST(test_page_init_sets_expected_header);
  RUN_TEST(test_page_add_record_updates_metadata_and_writes_payload);
  RUN_TEST(test_page_add_record_fails_when_page_has_no_space);
  RUN_TEST(test_page_delete_row_marks_tombstone_byte);

  // Pager
  RUN_TEST(test_pager_sets_page_count_to_zero);
  RUN_TEST(test_pager_create_page_increments_page_count);
  RUN_TEST(test_pager_close_flushed_modified_page_bytes);
  RUN_TEST(test_pager_create_page_assigns_sequential_ids);
  RUN_TEST(test_pager_page_count_preserved_after_reopen);

  return UNITY_END();
}
