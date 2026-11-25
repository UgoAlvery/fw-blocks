#include <stdint.h>
#include <string.h>

#include "fwb/antireplay/antireplay.h"
#include "unity.h"

void setUp(void)  {}
void tearDown(void) {}

static fwb_antireplay_ctx_t make_seeded(uint16_t seed)
{
    fwb_antireplay_ctx_t ctx;
    (void)fwb_antireplay_init(&ctx);
    (void)fwb_antireplay_check(&ctx, seed);
    return ctx;
}

/* AR-REQ-020, AR-REQ-026: delta == 1 → FWB_OK, mask shifted, last_seq updated */
static void test_delta_1_ok(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(10U);
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 11U));
}

/* AR-REQ-025: delta > 1 → FWB_ERR_SEQ_GAP but accepted (last_seq updated). */
static void test_delta_gt1_seq_gap(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(10U);
    TEST_ASSERT_EQUAL_INT(FWB_ERR_SEQ_GAP, fwb_antireplay_check(&ctx, 15U));
    /* Accepted: delta=1 from new last_seq=15 is now FWB_OK. */
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 16U));
}

/* AR-REQ-021: delta == 0 → replay */
static void test_delta_0_replay(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(10U);
    TEST_ASSERT_EQUAL_INT(FWB_ERR_REPLAY, fwb_antireplay_check(&ctx, 10U));
}

/* AR-REQ-022: in-window, bit not set → accepted */
static void test_in_window_not_seen(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(10U);
    (void)fwb_antireplay_check(&ctx, 11U); /* advance to 11 */
    /* seq=10 is now 1 behind last_seq=11, bit not set (we went forward). */
    /* Actually bit 1 (position for last_seq - 1 = 10) should be set because
     * when we seeded at 10, bit 0 was set, then shifting by 1 put it at bit 1.
     * Let me use a gap to have an unseen in-window value. */
    (void)fwb_antireplay_reset(&ctx);
    (void)fwb_antireplay_check(&ctx, 10U);
    (void)fwb_antireplay_check(&ctx, 12U); /* skip 11, delta=2 → SEQ_GAP */
    /* seq=11 is in window (delta=-1), bit not set */
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_antireplay_check(&ctx, 11U));
}

/* AR-REQ-023: in-window, bit already set → replay */
static void test_in_window_already_seen(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(10U);
    (void)fwb_antireplay_check(&ctx, 12U); /* advance, skip 11 */
    (void)fwb_antireplay_check(&ctx, 11U); /* accept 11 */
    TEST_ASSERT_EQUAL_INT(FWB_ERR_REPLAY, fwb_antireplay_check(&ctx, 11U));
}

/* AR-REQ-024: delta <= -W → too old */
static void test_too_old(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(100U);
    /* Advance by W so that seq=100 is now outside the window. */
    for (uint16_t i = 1U; i <= (uint16_t)FWB_ANTIREPLAY_WINDOW; i++) {
        (void)fwb_antireplay_check(&ctx, (uint16_t)(100U + i));
    }
    /* seq=100 is now exactly W behind last_seq, so delta = -W → too old. */
    TEST_ASSERT_EQUAL_INT(FWB_ERR_TOO_OLD, fwb_antireplay_check(&ctx, 100U));
}

/* AR-REQ-027: rejected value does not modify ctx. */
static void test_reject_no_state_change(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(10U);
    fwb_antireplay_ctx_t saved = ctx;

    /* Replay of seed value. */
    (void)fwb_antireplay_check(&ctx, 10U);
    TEST_ASSERT_EQUAL_MEMORY(&saved, &ctx, sizeof(ctx));

    /* Too old. */
    (void)fwb_antireplay_check(&ctx, (uint16_t)(10U - FWB_ANTIREPLAY_WINDOW));
    TEST_ASSERT_EQUAL_MEMORY(&saved, &ctx, sizeof(ctx));
}

/* AR-REQ-028: delta >= W → mask cleared entirely, no UB shift. */
static void test_large_advance_clears_mask(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(10U);
    /* Accept several in-window values. */
    (void)fwb_antireplay_check(&ctx, 12U);
    (void)fwb_antireplay_check(&ctx, 11U);

    /* Jump by exactly W: old history gone. */
    uint16_t jump = (uint16_t)(12U + (uint16_t)FWB_ANTIREPLAY_WINDOW);
    TEST_ASSERT_EQUAL_INT(FWB_ERR_SEQ_GAP, fwb_antireplay_check(&ctx, jump));

    /* seq=12 is now W steps behind → too old. */
    TEST_ASSERT_EQUAL_INT(FWB_ERR_TOO_OLD, fwb_antireplay_check(&ctx, 12U));
}

/* AR-REQ-028: delta == W-1 (stays in-window, largest valid shift). */
static void test_advance_w_minus_1(void)
{
    fwb_antireplay_ctx_t ctx = make_seeded(0U);
    uint16_t target = (uint16_t)(FWB_ANTIREPLAY_WINDOW - 1U);
    TEST_ASSERT_EQUAL_INT(FWB_ERR_SEQ_GAP, fwb_antireplay_check(&ctx, target));
    /* seq=0 is now W-1 behind, still in window (delta == -(W-1) > -W). */
    TEST_ASSERT_EQUAL_INT(FWB_ERR_REPLAY, fwb_antireplay_check(&ctx, 0U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_delta_1_ok);
    RUN_TEST(test_delta_gt1_seq_gap);
    RUN_TEST(test_delta_0_replay);
    RUN_TEST(test_in_window_not_seen);
    RUN_TEST(test_in_window_already_seen);
    RUN_TEST(test_too_old);
    RUN_TEST(test_reject_no_state_change);
    RUN_TEST(test_large_advance_clears_mask);
    RUN_TEST(test_advance_w_minus_1);
    return UNITY_END();
}
