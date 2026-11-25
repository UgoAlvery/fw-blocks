#include <stdint.h>

#include "fwb/antireplay/antireplay.h"
#include "unity.h"

void setUp(void)  {}
void tearDown(void) {}

/* AR-REQ-005 */
static void test_window_default(void)
{
    TEST_ASSERT_EQUAL_UINT(64U, FWB_ANTIREPLAY_WINDOW);
}

/* AR-REQ-004: null ctx → INVALID_ARG */
static void test_init_null(void)
{
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_antireplay_init(NULL));
}

/* AR-REQ-004 */
static void test_check_null(void)
{
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_antireplay_check(NULL, 0U));
}

/* AR-REQ-004 */
static void test_reset_null(void)
{
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_antireplay_reset(NULL));
}

/* AR-REQ-004: is_seeded returns false for null (documented safe default). */
static void test_is_seeded_null(void)
{
    TEST_ASSERT_FALSE(fwb_antireplay_is_seeded(NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_window_default);
    RUN_TEST(test_init_null);
    RUN_TEST(test_check_null);
    RUN_TEST(test_reset_null);
    RUN_TEST(test_is_seeded_null);
    return UNITY_END();
}
