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

/* CRC-REQ-010, CRC-REQ-011 : width 8 */
static void test_width8_smbus(void)
{
    static const fwb_crc_params_t p = {
        .width = 8U,
        .poly = 0x07U,
        .init = 0x00U,
        .refin = false,
        .refout = false,
        .xorout = 0x00U,
    };
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0xF4U, result);
}

/* CRC-REQ-011, CRC-REQ-014 : width 16 (predefined constant) */
static void test_width16_ccitt_false(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
                          fwb_crc_oneshot(&fwb_crc_params_ccitt_false, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0x29B1U, result);
}

/* CRC-REQ-011, CRC-REQ-015 : width 32 (predefined constant) */
static void test_width32_crc32c(void)
{
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&fwb_crc_params_crc32c, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0xE3069283U, result);
}

/* CRC-REQ-012 : refin et refout activables independamment */
static void test_refin1_refout0(void)
{
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x1021U,
        .init = 0xB2AAU,
        .refin = true,
        .refout = false,
        .xorout = 0x0000U,
    };
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, msg, MSG_LEN, &result));
    TEST_ASSERT_EQUAL_HEX32(0x0BC6U, result);
}

/* CRC-REQ-013 : polynome en forme normale (tient dans width bits, bit de poids fort implicite) */
static void test_poly_normal_form(void)
{
    /* Shift by width would be UB for width==32; only test 16-bit case. */
    TEST_ASSERT_EQUAL_HEX32(0U,
                            fwb_crc_params_ccitt_false.poly >> fwb_crc_params_ccitt_false.width);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_width8_smbus);
    RUN_TEST(test_width16_ccitt_false);
    RUN_TEST(test_width32_crc32c);
    RUN_TEST(test_refin1_refout0);
    RUN_TEST(test_poly_normal_form);
    return UNITY_END();
}
