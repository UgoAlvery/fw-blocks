#include <stdint.h>

#include "fwb/antireplay/antireplay.h"

#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

/* AR-REQ-030: 0xFFFF → 0x0000 treated as normal delta=+1. */
static void test_wrap_ffff_to_0000(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    (void)fwb_antireplay_check(&ctx, 0xFFFFU);
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 0x0000U));
}

/* AR-REQ-030: 0x0000 → 0xFFFF is a large backward delta (too old after wrap). */
static void test_backward_across_wrap(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    (void)fwb_antireplay_check(&ctx, 10U);
    /* 0xFFFF is (int16_t)(0xFFFF - 0x000A) = (int16_t)(0xFFF5) = -11 behind */
    /* For W=64, -11 > -64, so it should be in window (bit not set) → accepted. */
    fwb_status_t st = fwb_antireplay_check(&ctx, 0xFFFFU);
    /* -11 is within window (W=64): accepted but bit set.
     * For W=8: -11 <= -8 → too old. */
    if (FWB_ANTIREPLAY_WINDOW >= 16U) {
        TEST_ASSERT_EQUAL_INT(FWB_OK, st);
    } else {
        TEST_ASSERT_EQUAL_INT(FWB_ERR_TOO_OLD, st);
    }
}

/* AR-REQ-031: correct for any real gap < 32768. */
static void test_large_forward_gap(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    (void)fwb_antireplay_check(&ctx, 0U);
    /* Jump of 32767 (max positive int16_t delta). */
    TEST_ASSERT_EQUAL_INT(FWB_ERR_SEQ_GAP, fwb_antireplay_check(&ctx, 32767U));
}

/* AR-REQ-033: 70000 consecutive values, more than one full 16-bit cycle. */
static void test_70000_consecutive(void)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);

    for (uint32_t i = 0U; i < 70000U; i++) {
        uint16_t seq = (uint16_t)(i & 0xFFFFU);
        fwb_status_t st = fwb_antireplay_check(&ctx, seq);
        /* Every step is delta=1 (or delta=1 after wrap), so FWB_OK. */
        TEST_ASSERT_EQUAL_INT_MESSAGE(FWB_OK, st, "rejected in 70k sequence");
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_wrap_ffff_to_0000);
    RUN_TEST(test_backward_across_wrap);
    RUN_TEST(test_large_forward_gap);
    RUN_TEST(test_70000_consecutive);
    return UNITY_END();
}
