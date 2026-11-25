#include <stddef.h>
#include <stdint.h>

#include "fwb/crc/crc.h"

#include "unity.h"

static const fwb_crc_params_t valid = {
    .width = 32U,
    .poly = 0x04C11DB7U,
    .init = 0xFFFFFFFFU,
    .refin = true,
    .refout = true,
    .xorout = 0xFFFFFFFFU,
};
static const fwb_crc_params_t bad_low = {.width = 7U};
static const fwb_crc_params_t bad_high = {.width = 33U};
static const uint8_t data[] = {0xAAU, 0xBBU};

void setUp(void)
{
}
void tearDown(void)
{
}

/* CRC-REQ-003 */
static void test_init_null_ctx(void)
{
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_init(NULL, &valid));
}

/* CRC-REQ-003 */
static void test_init_null_params(void)
{
    fwb_crc_ctx_t ctx;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_init(&ctx, NULL));
}

/* CRC-REQ-003 */
static void test_init_width_too_small(void)
{
    fwb_crc_ctx_t ctx;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_init(&ctx, &bad_low));
}

/* CRC-REQ-003 */
static void test_init_width_too_large(void)
{
    fwb_crc_ctx_t ctx;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_init(&ctx, &bad_high));
}

/* CRC-REQ-003 */
static void test_update_null_ctx(void)
{
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_update(NULL, data, sizeof(data)));
}

/* CRC-REQ-003 */
static void test_update_null_data_nonzero_len(void)
{
    fwb_crc_ctx_t ctx;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, &valid));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_update(&ctx, NULL, 1U));
}

/* CRC-REQ-003 */
static void test_update_null_data_zero_len(void)
{
    fwb_crc_ctx_t ctx;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, &valid));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, NULL, 0U));
}

/* CRC-REQ-003 */
static void test_finalize_null_ctx(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_finalize(NULL, &result));
}

/* CRC-REQ-003 */
static void test_finalize_null_result(void)
{
    fwb_crc_ctx_t ctx;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, &valid));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_finalize(&ctx, NULL));
}

/* CRC-REQ-003 */
static void test_oneshot_null_params(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_oneshot(NULL, data, sizeof(data), &result));
}

/* CRC-REQ-003 */
static void test_oneshot_null_result(void)
{
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_oneshot(&valid, data, sizeof(data), NULL));
}

/* CRC-REQ-003 */
static void test_oneshot_null_data_nonzero_len(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_oneshot(&valid, NULL, 1U, &result));
}

/* CRC-REQ-003 */
static void test_oneshot_null_data_zero_len(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&valid, NULL, 0U, &result));
}

/* CRC-REQ-003 */
static void test_oneshot_width_too_small(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
                          fwb_crc_oneshot(&bad_low, data, sizeof(data), &result));
}

/* CRC-REQ-003 */
static void test_oneshot_width_too_large(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
                          fwb_crc_oneshot(&bad_high, data, sizeof(data), &result));
}

/* CRC-REQ-004, CRC-REQ-052 */
static void test_unaligned_ccitt_false(void)
{
    static const uint8_t padded[10] = {0x00U, '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    static const uint8_t direct[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint32_t r_unaligned = 0U;
    uint32_t r_direct = 0U;
    TEST_ASSERT_EQUAL_INT(
        FWB_OK, fwb_crc_oneshot(&fwb_crc_params_ccitt_false, padded + 1U, 9U, &r_unaligned));
    TEST_ASSERT_EQUAL_INT(FWB_OK,
                          fwb_crc_oneshot(&fwb_crc_params_ccitt_false, direct, 9U, &r_direct));
    TEST_ASSERT_EQUAL_HEX32(r_direct, r_unaligned);
}

/* CRC-REQ-004, CRC-REQ-052 */
static void test_unaligned_crc32c(void)
{
    static const uint8_t padded[10] = {0x00U, '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    static const uint8_t direct[9] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    uint32_t r_unaligned = 0U;
    uint32_t r_direct = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
                          fwb_crc_oneshot(&fwb_crc_params_crc32c, padded + 1U, 9U, &r_unaligned));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&fwb_crc_params_crc32c, direct, 9U, &r_direct));
    TEST_ASSERT_EQUAL_HEX32(r_direct, r_unaligned);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_init_null_ctx);
    RUN_TEST(test_init_null_params);
    RUN_TEST(test_init_width_too_small);
    RUN_TEST(test_init_width_too_large);
    RUN_TEST(test_update_null_ctx);
    RUN_TEST(test_update_null_data_nonzero_len);
    RUN_TEST(test_update_null_data_zero_len);
    RUN_TEST(test_finalize_null_ctx);
    RUN_TEST(test_finalize_null_result);
    RUN_TEST(test_oneshot_null_params);
    RUN_TEST(test_oneshot_null_result);
    RUN_TEST(test_oneshot_null_data_nonzero_len);
    RUN_TEST(test_oneshot_null_data_zero_len);
    RUN_TEST(test_oneshot_width_too_small);
    RUN_TEST(test_oneshot_width_too_large);
    RUN_TEST(test_unaligned_ccitt_false);
    RUN_TEST(test_unaligned_crc32c);
    return UNITY_END();
}
