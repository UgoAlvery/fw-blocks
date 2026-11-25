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
    TEST_ASSERT_EQUAL_HEX16(0x1234U, fwb_rd_u16_le(buf));
}

static void test_wr_u16_le(void)
{
    uint8_t buf[2] = {0};
    fwb_wr_u16_le(buf, 0xABCDU);
    TEST_ASSERT_EQUAL_HEX8(0xCDU, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xABU, buf[1]);
}

static void test_rd_u32_le(void)
{
    const uint8_t buf[] = {0x78, 0x56, 0x34, 0x12};
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, fwb_rd_u32_le(buf));
}

static void test_wr_u32_le(void)
{
    uint8_t buf[4] = {0};
    fwb_wr_u32_le(buf, 0xDEADBEEFU);
    TEST_ASSERT_EQUAL_HEX8(0xEFU, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xBEU, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0xADU, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xDEU, buf[3]);
}

static void test_rd_u16_be(void)
{
    const uint8_t buf[] = {0x12, 0x34};
    TEST_ASSERT_EQUAL_HEX16(0x1234U, fwb_rd_u16_be(buf));
}

static void test_wr_u16_be(void)
{
    uint8_t buf[2] = {0};
    fwb_wr_u16_be(buf, 0xABCDU);
    TEST_ASSERT_EQUAL_HEX8(0xABU, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xCDU, buf[1]);
}

static void test_rd_u32_be(void)
{
    const uint8_t buf[] = {0x12, 0x34, 0x56, 0x78};
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, fwb_rd_u32_be(buf));
}

static void test_wr_u32_be(void)
{
    uint8_t buf[4] = {0};
    fwb_wr_u32_be(buf, 0xDEADBEEFU);
    TEST_ASSERT_EQUAL_HEX8(0xDEU, buf[0]);
    TEST_ASSERT_EQUAL_HEX8(0xADU, buf[1]);
    TEST_ASSERT_EQUAL_HEX8(0xBEU, buf[2]);
    TEST_ASSERT_EQUAL_HEX8(0xEFU, buf[3]);
}

static void test_aller_retour_u16_le(void)
{
    uint8_t buf[2];
    uint16_t val = 0xCAFEU;
    fwb_wr_u16_le(buf, val);
    TEST_ASSERT_EQUAL_HEX16(val, fwb_rd_u16_le(buf));
}

static void test_aller_retour_u32_le(void)
{
    uint8_t buf[4];
    uint32_t val = 0xDEADC0DEU;
    fwb_wr_u32_le(buf, val);
    TEST_ASSERT_EQUAL_HEX32(val, fwb_rd_u32_le(buf));
}

static void test_non_aligne_u16(void)
{
    /* Acces non aligne : offset 1 dans un tableau uint8_t */
    uint8_t buf[3] = {0xFF, 0x34, 0x12};
    TEST_ASSERT_EQUAL_HEX16(0x1234U, fwb_rd_u16_le(buf + 1));
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
