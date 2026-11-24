#include <stddef.h>
#include <stdint.h>

#include "fwb/crc/crc.h"

#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

/* CRC-REQ-021 */
static void test_crc32_iso_hdlc(void)
{
    static const uint8_t message[] = "123456789";
    static const fwb_crc_params_t p = {
        .width = 32U,
        .poly = 0x04C11DB7U,
        .init = 0xFFFFFFFFU,
        .refin = true,
        .refout = true,
        .xorout = 0xFFFFFFFFU,
    };
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, message, sizeof(message) - 1U, &result));
    TEST_ASSERT_EQUAL_HEX32(0xCBF43926U, result);
}

/* CRC-REQ-020 */
static void test_crc16_arc(void)
{
    static const uint8_t message[] = "123456789";
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x8005U,
        .init = 0x0000U,
        .refin = true,
        .refout = true,
        .xorout = 0x0000U,
    };
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, message, sizeof(message) - 1U, &result));
    TEST_ASSERT_EQUAL_HEX32(0x0000BB3DU, result);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_crc32_iso_hdlc);
    RUN_TEST(test_crc16_arc);
    return UNITY_END();
}
