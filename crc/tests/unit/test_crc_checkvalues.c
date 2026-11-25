#include <stddef.h>
#include <stdint.h>

#include "fwb/crc/crc.h"

#include "unity.h"

static const uint8_t msg[] = "123456789";
#define MSG_LEN (sizeof(msg) - 1U)

void setUp(void)
{
}
void tearDown(void)
{
}

/* CRC-REQ-020 */
static void test_crc32_iso_hdlc(void)
{
    static const fwb_crc_params_t p = {
        .width = 32U,
        .poly = 0x04C11DB7U,
        .init = 0xFFFFFFFFU,
        .refin = true,
        .refout = true,
        .xorout = 0xFFFFFFFFU,
    };
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926U, result);
}

/* CRC-REQ-020 */
static void test_crc16_arc(void)
{
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x8005U,
        .init = 0x0000U,
        .refin = true,
        .refout = true,
        .xorout = 0x0000U,
    };
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0x0000BB3DU, result);
}

/* CRC-REQ-020 */
static void test_ccitt_false(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
                          fwb_crc_oneshot(&fwb_crc_params_ccitt_false, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0x000029B1U, result);
}

/* CRC-REQ-021 */
static void test_crc32c(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&fwb_crc_params_crc32c, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0xE3069283U, result);
}

/* CRC-REQ-020 */
static void test_crc16_tms37157(void)
{
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x1021U,
        .init = 0x89ECU,
        .refin = true,
        .refout = true,
        .xorout = 0x0000U,
    };
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0x000026B1U, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc32_iso_hdlc);
    RUN_TEST(test_crc16_arc);
    RUN_TEST(test_ccitt_false);
    RUN_TEST(test_crc32c);
    RUN_TEST(test_crc16_tms37157);
    return UNITY_END();
}
