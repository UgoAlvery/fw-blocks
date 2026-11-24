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

static uint32_t next_rand(uint32_t *state)
{
    *state = (*state * 1664525U) + 1013904223U;
    return *state >> 8U;
}

static void check_equiv(const fwb_crc_params_t *params)
{
    fwb_crc_table_t table;
    uint32_t seed = 0x12345678U;
    uint8_t buf[300];

    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_table_build(params, &table));

    for (unsigned iter = 0U; iter < 200U; iter++) {
        const size_t len = next_rand(&seed) % sizeof(buf);
        const size_t cut = (len == 0U) ? 0U : (next_rand(&seed) % (len + 1U));
        fwb_crc_ctx_t ca;
        fwb_crc_ctx_t cb;
        uint32_t ra = 0U;
        uint32_t rb = 0U;

        for (size_t i = 0U; i < len; i++) {
            buf[i] = (uint8_t)next_rand(&seed);
        }

        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ca, params));
        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&cb, params));
        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update(&ca, buf, len));
        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update_table(&cb, &table, buf, cut));
        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_update_table(&cb, &table, buf + cut, len - cut));
        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&ca, &ra));
        TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_finalize(&cb, &rb));
        TEST_ASSERT_EQUAL_HEX32(ra, rb);
    }
}

/* CRC-REQ-040 */
static void test_equiv_ccitt_false(void)
{
    check_equiv(&fwb_crc_params_ccitt_false);
}
/* CRC-REQ-040 */
static void test_equiv_crc32c(void)
{
    check_equiv(&fwb_crc_params_crc32c);
}

/* CRC-REQ-040 */
static void test_equiv_crc8_smbus(void)
{
    static const fwb_crc_params_t p = {
        .width = 8U,
        .poly = 0x07U,
        .init = 0x00U,
        .refin = false,
        .refout = false,
        .xorout = 0x00U,
    };
    check_equiv(&p);
}

/* CRC-REQ-040 */
static void test_equiv_crc32_iso_hdlc(void)
{
    static const fwb_crc_params_t p = {
        .width = 32U,
        .poly = 0x04C11DB7U,
        .init = 0xFFFFFFFFU,
        .refin = true,
        .refout = true,
        .xorout = 0xFFFFFFFFU,
    };
    check_equiv(&p);
}

/* CRC-REQ-040 */
static void test_equiv_crc32_bzip2(void)
{
    static const fwb_crc_params_t p = {
        .width = 32U,
        .poly = 0x04C11DB7U,
        .init = 0xFFFFFFFFU,
        .refin = false,
        .refout = false,
        .xorout = 0xFFFFFFFFU,
    };
    check_equiv(&p);
}

/* CRC-REQ-040 */
static void test_equiv_crc16_tms37157(void)
{
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x1021U,
        .init = 0x89ECU,
        .refin = true,
        .refout = true,
        .xorout = 0x0000U,
    };
    check_equiv(&p);
}

/* CRC-REQ-040 */
static void test_equiv_crc16_riello_asym(void)
{
    static const fwb_crc_params_t p = {
        .width = 16U,
        .poly = 0x1021U,
        .init = 0xB2AAU,
        .refin = true,
        .refout = false,
        .xorout = 0x0000U,
    };
    check_equiv(&p);
}

/* CRC-REQ-040 */
static void test_equiv_width12(void)
{
    static const fwb_crc_params_t p = {
        .width = 12U,
        .poly = 0x80FU,
        .init = 0xFFFU,
        .refin = false,
        .refout = true,
        .xorout = 0x000U,
    };
    check_equiv(&p);
}

/* CRC-REQ-040 */
static void test_equiv_width24(void)
{
    static const fwb_crc_params_t p = {
        .width = 24U,
        .poly = 0x864CFBU,
        .init = 0xB704CEU,
        .refin = true,
        .refout = false,
        .xorout = 0x000000U,
    };
    check_equiv(&p);
}

/* CRC-REQ-041 : table/ctx mismatch rejete */
static void test_table_mismatch_rejected(void)
{
    fwb_crc_table_t table;
    fwb_crc_ctx_t ctx;
    static const uint8_t d[1] = {0U};
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_table_build(&fwb_crc_params_crc32c, &table));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, &fwb_crc_params_ccitt_false));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_update_table(&ctx, &table, d, 1U));
}

/* CRC-REQ-003 (API table) */
static void test_table_null_args_rejected(void)
{
    fwb_crc_table_t table;
    fwb_crc_ctx_t ctx;
    static const uint8_t d[1] = {0U};
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_table_build(&fwb_crc_params_crc32c, &table));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_crc_init(&ctx, &fwb_crc_params_ccitt_false));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_table_build(NULL, &table));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_table_build(&fwb_crc_params_crc32c, NULL));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_update_table(NULL, &table, d, 1U));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_crc_update_table(&ctx, NULL, d, 1U));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_equiv_ccitt_false);
    RUN_TEST(test_equiv_crc32c);
    RUN_TEST(test_equiv_crc8_smbus);
    RUN_TEST(test_equiv_crc32_iso_hdlc);
    RUN_TEST(test_equiv_crc32_bzip2);
    RUN_TEST(test_equiv_crc16_tms37157);
    RUN_TEST(test_equiv_crc16_riello_asym);
    RUN_TEST(test_equiv_width12);
    RUN_TEST(test_equiv_width24);
    RUN_TEST(test_table_mismatch_rejected);
    RUN_TEST(test_table_null_args_rejected);
    return UNITY_END();
}
