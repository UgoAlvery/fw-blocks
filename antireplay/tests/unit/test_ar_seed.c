#include <stdint.h>
#include <string.h>

#include "fwb/antireplay/antireplay.h"

#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

/* AR-REQ-010 */
static void test_init_not_seeded(void)
{
    fwb_antireplay_ctx_t ctx;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_init(&ctx));
    TEST_ASSERT_FALSE(fwb_antireplay_is_seeded(&ctx));
}

/* AR-REQ-013 */
static void test_is_seeded_queryable_before_check(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    /* Should be callable without crashing; value already tested above. */
    (void)fwb_antireplay_is_seeded(&ctx);
    TEST_PASS();
}

/* AR-REQ-011, 012: first value accepted, context becomes seeded. */
static void test_first_value_accepted(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 42U));
    TEST_ASSERT_TRUE(fwb_antireplay_is_seeded(&ctx));
}

/* AR-REQ-011: first value = 0 also accepted. */
static void test_first_value_zero(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 0U));
    TEST_ASSERT_TRUE(fwb_antireplay_is_seeded(&ctx));
}

/* AR-REQ-012: last_seq fixed to first accepted value.
 * Verify by submitting seq+1 → delta=1 → FWB_OK (not SEQ_GAP). */
static void test_seed_fixes_last_seq(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    (void)fwb_antireplay_check(&ctx, 100U);
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 101U));
}

/* AR-REQ-014: reset returns to unseeded state. */
static void test_reset_clears_seeded(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    (void)fwb_antireplay_check(&ctx, 10U);
    TEST_ASSERT_TRUE(fwb_antireplay_is_seeded(&ctx));

    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_reset(&ctx));
    TEST_ASSERT_FALSE(fwb_antireplay_is_seeded(&ctx));
}

/* AR-REQ-014: after reset, any first value is accepted again. */
static void test_reset_allows_new_seed(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    (void)fwb_antireplay_check(&ctx, 10U);
    (void)fwb_antireplay_reset(&ctx);

    /* Value that would have been TOO_OLD before reset. */
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 0U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_not_seeded);
    RUN_TEST(test_is_seeded_queryable_before_check);
    RUN_TEST(test_first_value_accepted);
    RUN_TEST(test_first_value_zero);
    RUN_TEST(test_seed_fixes_last_seq);
    RUN_TEST(test_reset_clears_seeded);
    RUN_TEST(test_reset_allows_new_seed);
    return UNITY_END();
}
