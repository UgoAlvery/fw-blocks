#include <stdint.h>

#include "fwb/common/endian.h"

#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

static void test_rd_u16_le(void)
{
    const uint8_t buf[] = {0x34, 0x12};
    TEST_ASSERT_EQUAL_HEX16(0x1234u, fwb_rd_u16_le(buf));
}

static void test_wr_u16_le(void)
{
    uint8_t buf[2] = {0};
    fwb_wr_u16_le(buf, 0xABCDu);
    TEST_ASSERT_EQUAL_HEX8(0xCDu, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xABu, buf[1]);
}

static void test_rd_u32_le(void)
{
    const uint8_t buf[] = {0x78, 0x56, 0x34, 0x12};
    TEST_ASSERT_EQUAL_HEX32(0x12345678u, fwb_rd_u32_le(buf));
}

static void test_wr_u32_le(void)
{
    uint8_t buf[4] = {0};
    fwb_wr_u32_le(buf, 0xDEADBEEFu);
    TEST_ASSERT_EQUAL_HEX8(0xEFu, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBEu, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0xADu, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xDEu, buf[3]);
}

static void test_rd_u16_be(void)
{
    const uint8_t buf[] = {0x12, 0x34};
    TEST_ASSERT_EQUAL_HEX16(0x1234u, fwb_rd_u16_be(buf));
}

static void test_wr_u16_be(void)
{
    uint8_t buf[2] = {0};
    fwb_wr_u16_be(buf, 0xABCDu);
    TEST_ASSERT_EQUAL_HEX8(0xABu, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xCDu, buf[1]);
}

static void test_rd_u32_be(void)
{
    const uint8_t buf[] = {0x12, 0x34, 0x56, 0x78};
    TEST_ASSERT_EQUAL_HEX32(0x12345678u, fwb_rd_u32_be(buf));
}

static void test_wr_u32_be(void)
{
    uint8_t buf[4] = {0};
    fwb_wr_u32_be(buf, 0xDEADBEEFu);
    TEST_ASSERT_EQUAL_HEX8(0xDEu, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xADu, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBEu, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xEFu, buf[3]);
}

static void test_aller_retour_u16_le(void)
{
    uint8_t buf[2];
    uint16_t val = 0xCAFEu;
    fwb_wr_u16_le(buf, val);
    TEST_ASSERT_EQUAL_HEX16(val, fwb_rd_u16_le(buf));
}

static void test_aller_retour_u32_le(void)
{
    uint8_t buf[4];
    uint32_t val = 0xDEADC0DEu;
    fwb_wr_u32_le(buf, val);
    TEST_ASSERT_EQUAL_HEX32(val, fwb_rd_u32_le(buf));
}

static void test_non_aligne_u16(void)
{
    /* Acces non aligne : offset 1 dans un tableau uint8_t */
    uint8_t buf[3] = {0xFF, 0x34, 0x12};
    TEST_ASSERT_EQUAL_HEX16(0x1234u, fwb_rd_u16_le(buf + 1));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rd_u16_le);
    RUN_TEST(test_wr_u16_le);
    RUN_TEST(test_rd_u32_le);
    RUN_TEST(test_wr_u32_le);
    RUN_TEST(test_rd_u16_be);
    RUN_TEST(test_wr_u16_be);
    RUN_TEST(test_rd_u32_be);
    RUN_TEST(test_wr_u32_be);
    RUN_TEST(test_aller_retour_u16_le);
    RUN_TEST(test_aller_retour_u32_le);
    RUN_TEST(test_non_aligne_u16);
    return UNITY_END();
}
