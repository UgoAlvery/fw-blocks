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

static uint32_t reflect_bits(uint32_t value, unsigned width)
{
    uint32_t out = 0U;
    for (unsigned i = 0U; i < width; i++) {
        out = (out << 1U) | ((value >> i) & 1U);
    }
    return out;
}

/* CRC-REQ-014 */
static void test_ccitt_false_fields(void)
{
    const fwb_crc_params_t *p = &fwb_crc_params_ccitt_false;
    TEST_ASSERT_EQUAL_HEX32(16U, p->width);
    TEST_ASSERT_EQUAL_HEX32(0x1021U, p->poly);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFU, p->init);
    TEST_ASSERT_EQUAL_HEX32(0x0000U, p->xorout);
    TEST_ASSERT_FALSE(p->refin);
    TEST_ASSERT_FALSE(p->refout);
}

/* CRC-REQ-015 */
static void test_crc32c_fields(void)
{
    const fwb_crc_params_t *p = &fwb_crc_params_crc32c;
    TEST_ASSERT_EQUAL_HEX32(32U, p->width);
    TEST_ASSERT_EQUAL_HEX32(0x1EDC6F41U, p->poly);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, p->init);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, p->xorout);
    TEST_ASSERT_TRUE(p->refin);
    TEST_ASSERT_TRUE(p->refout);
}

/* CRC-REQ-013 */
static void test_poly_fits_in_width(void)
{
    /* Shift by width is UB for width==32; only test the 16-bit constant. */
    TEST_ASSERT_EQUAL_HEX32(0U,
                            fwb_crc_params_ccitt_false.poly >> fwb_crc_params_ccitt_false.width);
}

static void check_empty_input(bool refin, bool refout)
{
    const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x1021U,
        .init = 0x1234U,
        .xorout = 0x00FFU,
        .refin = refin,
        .refout = refout,
    };
    const uint32_t base = refout ? reflect_bits(p.init, 16U) : p.init;
    const uint32_t expected = (base ^ p.xorout) & 0xFFFFU;
    uint32_t result = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&p, NULL, 0U, &result));
    TEST_ASSERT_EQUAL_HEX32(expected, result);
}

/* CRC-REQ-033 */
static void test_empty_refin0_refout0(void)
{
    check_empty_input(false, false);
}
/* CRC-REQ-033 */
static void test_empty_refin1_refout0(void)
{
    check_empty_input(true, false);
}
/* CRC-REQ-033 */
static void test_empty_refin0_refout1(void)
{
    check_empty_input(false, true);
}
/* CRC-REQ-033 */
static void test_empty_refin1_refout1(void)
{
    check_empty_input(true, true);
}

/* CRC-REQ-034 */
static void test_finalize_is_pure(void)
{
    static const uint8_t msg[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    fwb_crc_ctx_t ctx;
    uint32_t r1 = 0U;
    uint32_t r2 = 0U;
    uint32_t full = 0U;
    uint32_t after = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, &fwb_crc_params_crc32c));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, msg, 4U));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &r1));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &r2));
    TEST_ASSERT_EQUAL_HEX32(r1, r2);
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, msg + 4, 5U));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &after));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(&fwb_crc_params_crc32c, msg, 9U, &full));
    TEST_ASSERT_EQUAL_HEX32(0xE3069283U, after);
    TEST_ASSERT_EQUAL_HEX32(full, after);
}

static void check_oneshot_equiv(const fwb_crc_params_t *params)
{
    static const uint8_t msg[] = {0x80U, 0xA5U, 0xFFU, 0xC3U, 0x00U, 0x7FU, 0x01U};
    fwb_crc_ctx_t ctx;
    uint32_t one = 0U;
    uint32_t inc = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_oneshot(params, msg, sizeof(msg), &one));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, params));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ctx, msg, sizeof(msg)));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ctx, &inc));
    TEST_ASSERT_EQUAL_HEX32(one, inc);
}

/* CRC-REQ-035 */
static void test_oneshot_equiv_crc8(void)
{
    static const fwb_crc_params_t p = {
        .width = 8U,
        .poly = 0x07U,
        .init = 0x00U,
        .xorout = 0x00U,
        .refin = false,
        .refout = false,
    };
    check_oneshot_equiv(&p);
}

/* CRC-REQ-035 */
static void test_oneshot_equiv_crc16_tms(void)
{
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x1021U,
        .init = 0x89ECU,
        .xorout = 0x0000U,
        .refin = true,
        .refout = true,
    };
    check_oneshot_equiv(&p);
}

/* CRC-REQ-035 */
static void test_oneshot_equiv_crc16_riello_asym(void)
{
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x1021U,
        .init = 0xB2AAU,
        .xorout = 0x0000U,
        .refin = true,
        .refout = false,
    };
    check_oneshot_equiv(&p);
}

/* CRC-REQ-035 */
static void test_oneshot_equiv_crc32_refin(void)
{
    static const fwb_crc_params_t p = {
        .width = 32U,
        .poly = 0x04C11DB7U,
        .init = 0xFFFFFFFFU,
        .xorout = 0xFFFFFFFFU,
        .refin = true,
        .refout = true,
    };
    check_oneshot_equiv(&p);
}

/* CRC-REQ-035 */
static void test_oneshot_equiv_crc32_noref(void)
{
    static const fwb_crc_params_t p = {
        .width = 32U,
        .poly = 0x04C11DB7U,
        .init = 0xFFFFFFFFU,
        .xorout = 0xFFFFFFFFU,
        .refin = false,
        .refout = false,
    };
    check_oneshot_equiv(&p);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ccitt_false_fields);
    RUN_TEST(test_crc32c_fields);
    RUN_TEST(test_poly_fits_in_width);
    RUN_TEST(test_empty_refin0_refout0);
    RUN_TEST(test_empty_refin1_refout0);
    RUN_TEST(test_empty_refin0_refout1);
    RUN_TEST(test_empty_refin1_refout1);
    RUN_TEST(test_finalize_is_pure);
    RUN_TEST(test_oneshot_equiv_crc8);
    RUN_TEST(test_oneshot_equiv_crc16_tms);
    RUN_TEST(test_oneshot_equiv_crc16_riello_asym);
    RUN_TEST(test_oneshot_equiv_crc32_refin);
    RUN_TEST(test_oneshot_equiv_crc32_noref);
    return UNITY_END();
}
