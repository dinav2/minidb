#include "unity.h"

void setUp() {}

void tearDown() {}

extern void test_page_init_sets_expected_header(void);
extern void test_page_add_record_updates_metadata_and_writes_payload(void);

int main(void) {
  UNITY_BEGIN();

  RUN_TEST(test_page_init_sets_expected_header);
  RUN_TEST(test_page_add_record_updates_metadata_and_writes_payload);

  return UNITY_END();
}
