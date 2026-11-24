#include <stddef.h>
#include <stdint.h>

#include "fwb/crc/crc.h"

#include "unity.h"

static const uint8_t MSG[] = "123456789";
#define MSG_LEN (sizeof(MSG) - 1U)

void setUp(void)
{
}
void tearDown(void)
{
}

static uint32_t oneshot_ref(const fwb_crc_params_t *params, const uint8_t *data, size_t len)
{
    uint32_t r = 0U;
    (void)fwb_crc_oneshot(params, data, len, &r);
    return r;
}

static void check_split(const fwb_crc_params_t *params, size_t cut)
{
    fwb_crc_ctx_t ctx;
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, params));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, MSG, cut));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, MSG + cut, MSG_LEN - cut));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &result));
    TEST_ASSERT_EQUAL_HEX32(oneshot_ref(params, MSG, MSG_LEN), result);
}

static void check_byte_by_byte(const fwb_crc_params_t *params)
{
    fwb_crc_ctx_t ctx;
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, params));
    for (size_t i = 0U; i < MSG_LEN; i++) {
        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, MSG + i, 1U));
    }
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &result));
    TEST_ASSERT_EQUAL_HEX32(oneshot_ref(params, MSG, MSG_LEN), result);
}

static void check_intermediate_finalize(const fwb_crc_params_t *params)
{
    static const size_t CUT = 4U;
    fwb_crc_ctx_t ctx;
    uint32_t mid = 0U;
    uint32_t final_val = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, params));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, MSG, CUT));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &mid));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, MSG + CUT, MSG_LEN - CUT));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &final_val));
    TEST_ASSERT_EQUAL_HEX32(oneshot_ref(params, MSG, CUT), mid);
    TEST_ASSERT_EQUAL_HEX32(oneshot_ref(params, MSG, MSG_LEN), final_val);
}

static void check_zero_len_noop(const fwb_crc_params_t *params)
{
    fwb_crc_ctx_t ctx_a;
    fwb_crc_ctx_t ctx_b;
    uint32_t a = 0U;
    uint32_t b = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx_a, params));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx_a, MSG, MSG_LEN));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx_a, &a));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx_b, params));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx_b, MSG, 0U));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx_b, MSG, MSG_LEN));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx_b, &b));
    TEST_ASSERT_EQUAL_HEX32(a, b);
}

static void check_empty_sequence(const fwb_crc_params_t *params)
{
    fwb_crc_ctx_t ctx;
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, params));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &result));
    TEST_ASSERT_EQUAL_HEX32(oneshot_ref(params, NULL, 0U), result);
}

static const fwb_crc_params_t CRC32 = {
    .width = 32U,
    .poly = 0x04C11DB7U,
    .init = 0xFFFFFFFFU,
    .refin = true,
    .refout = true,
    .xorout = 0xFFFFFFFFU,
};

/* CRC-REQ-031 */
static void test_split_1_8_crc32(void)
{
    check_split(&CRC32, 1U);
}
/* CRC-REQ-031 */
static void test_split_4_5_crc32(void)
{
    check_split(&CRC32, 4U);
}
/* CRC-REQ-031 */
static void test_split_8_1_crc32(void)
{
    check_split(&CRC32, 8U);
}
/* CRC-REQ-030 */
static void test_byte_by_byte_crc32(void)
{
    check_byte_by_byte(&CRC32);
}
/* CRC-REQ-034 */
static void test_intermediate_finalize_crc32(void)
{
    check_intermediate_finalize(&CRC32);
}
/* CRC-REQ-032 */
static void test_zero_len_noop_crc32(void)
{
    check_zero_len_noop(&CRC32);
}
/* CRC-REQ-033 */
static void test_empty_sequence_crc32(void)
{
    check_empty_sequence(&CRC32);
}
/* CRC-REQ-031 */
static void test_split_4_5_ccitt_false(void)
{
    check_split(&fwb_crc_params_ccitt_false, 4U);
}
/* CRC-REQ-030 */
static void test_byte_by_byte_ccitt_false(void)
{
    check_byte_by_byte(&fwb_crc_params_ccitt_false);
}
/* CRC-REQ-034 */
static void test_intermediate_finalize_ccitt_false(void)
{
    check_intermediate_finalize(&fwb_crc_params_ccitt_false);
}
/* CRC-REQ-033 */
static void test_empty_sequence_ccitt_false(void)
{
    check_empty_sequence(&fwb_crc_params_ccitt_false);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_split_1_8_crc32);
    RUN_TEST(test_split_4_5_crc32);
    RUN_TEST(test_split_8_1_crc32);
    RUN_TEST(test_byte_by_byte_crc32);
    RUN_TEST(test_intermediate_finalize_crc32);
    RUN_TEST(test_zero_len_noop_crc32);
    RUN_TEST(test_empty_sequence_crc32);
    RUN_TEST(test_split_4_5_ccitt_false);
    RUN_TEST(test_byte_by_byte_ccitt_false);
    RUN_TEST(test_intermediate_finalize_ccitt_false);
    RUN_TEST(test_empty_sequence_ccitt_false);
    return UNITY_END();
}
