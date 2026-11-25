#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/framing/framing.h"
#include "unity.h"

void setUp(void)  {}
void tearDown(void) {}

static fwb_framing_cfg_t cfg_zero = {.link_id = 0x0000U};

/* FRM-REQ-010, FRM-REQ-011, FRM-REQ-012, FRM-REQ-013: basic encode, check wire bytes against FRM-V-002. */
static void test_encode_v002(void)
{
    /* TYPE=0x20, SEQ=1, payload="ABC", link_id=0x0000 → FRM-V-002. */
    static const uint8_t payload[] = {0x41U, 0x42U, 0x43U};
    static const uint8_t expected_wire[] = {
        0x02U, 0x01U, 0x03U, 0x20U, 0x01U, 0x02U, 0x03U,
        0x06U, 0x41U, 0x42U, 0x43U, 0xF3U, 0xFFU, 0x00U
    };
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;

    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x20U, 1U,
                           payload, sizeof(payload),
                           wire, sizeof(wire), &n));
    TEST_ASSERT_EQUAL_UINT(sizeof(expected_wire), n);
    TEST_ASSERT_EQUAL_MEMORY(expected_wire, wire, n);
}

/* FRM-REQ-010, FRM-REQ-020: empty payload → FRM-V-001. */
static void test_encode_v001_empty_payload(void)
{
    static const uint8_t expected_wire[] = {
        0x02U, 0x01U, 0x02U, 0x10U, 0x01U, 0x01U, 0x01U,
        0x03U, 0x49U, 0xA4U, 0x00U
    };
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;

    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x10U, 0U,
                           NULL, 0U, wire, sizeof(wire), &n));
    TEST_ASSERT_EQUAL_UINT(sizeof(expected_wire), n);
    TEST_ASSERT_EQUAL_MEMORY(expected_wire, wire, n);
}

/* FRM-REQ-019: SEQ passed through unchanged. */
static void test_encode_seq_unchanged(void)
{
    uint8_t wire1[FWB_FRAMING_MAX_WIRE_SIZE];
    uint8_t wire2[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n1 = 0U, n2 = 0U;

    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x01U, 42U,
                           NULL, 0U, wire1, sizeof(wire1), &n1));
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x01U, 42U,
                           NULL, 0U, wire2, sizeof(wire2), &n2));
    TEST_ASSERT_EQUAL_UINT(n1, n2);
    TEST_ASSERT_EQUAL_MEMORY(wire1, wire2, n1);
}

/* FRM-REQ-015: payload too large. */
static void test_encode_payload_too_large(void)
{
    uint8_t dst[FWB_FRAMING_MAX_WIRE_SIZE + 32U];
    uint8_t big[FWB_FRAMING_MAX_PAYLOAD + 1U];
    size_t  n = 0U;
    memset(big, 0xAAU, sizeof(big));

    TEST_ASSERT_EQUAL_INT(FWB_ERR_PAYLOAD_TOO_LARGE,
        fwb_framing_encode(&cfg_zero, 0U, 0U,
                           big, (uint16_t)sizeof(big),
                           dst, sizeof(dst), &n));
}

/* FRM-REQ-016: buffer too small → no bytes written. */
static void test_encode_buffer_too_small(void)
{
    uint8_t small[4];
    size_t  n = 0xDEADBEEFU;
    static const uint8_t sentinel = 0xCCU;
    memset(small, (int)sentinel, sizeof(small));

    fwb_status_t st = fwb_framing_encode(&cfg_zero, 0U, 0U,
                                         NULL, 0U, small, sizeof(small), &n);
    TEST_ASSERT_EQUAL_INT(FWB_ERR_BUFFER_TOO_SMALL, st);
    /* Buffer must be untouched. */
    for (size_t i = 0U; i < sizeof(small); i++) {
        TEST_ASSERT_EQUAL_HEX8(sentinel, small[i]);
    }
}

/* FRM-REQ-017: null payload with non-zero length → INVALID_ARG. */
static void test_encode_null_payload_nonzero_len(void)
{
    uint8_t dst[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
        fwb_framing_encode(&cfg_zero, 0U, 0U,
                           NULL, 1U, dst, sizeof(dst), &n));
}

/* FRM-REQ-018: payload buffer not modified. */
static void test_encode_payload_not_modified(void)
{
    static const uint8_t original[] = {0xAAU, 0xBBU, 0xCCU};
    uint8_t payload[sizeof(original)];
    memcpy(payload, original, sizeof(original));

    uint8_t dst[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x01U, 0U,
                           payload, sizeof(payload),
                           dst, sizeof(dst), &n));
    TEST_ASSERT_EQUAL_MEMORY(original, payload, sizeof(original));
}

/* FRM-REQ-021: no 0x00 before delimiter. */
static void test_encode_no_zero_before_delimiter(void)
{
    static const uint8_t zeros[8] = {0};
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;

    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0U, 0U,
                           zeros, sizeof(zeros),
                           wire, sizeof(wire), &n));
    for (size_t i = 0U; i + 1U < n; i++) {
        TEST_ASSERT_NOT_EQUAL_MESSAGE(0U, wire[i], "zero before delimiter");
    }
    TEST_ASSERT_EQUAL_HEX8(0x00U, wire[n - 1U]);
}

/* FRM-REQ-014: same wire bytes except CRC with link_id=0xBEEF → FRM-V-006. */
static void test_encode_link_id(void)
{
    static const uint8_t payload[] = {0x41U, 0x42U, 0x43U};
    static const uint8_t expected_wire[] = {
        0x02U, 0x01U, 0x03U, 0x20U, 0x01U, 0x02U, 0x03U,
        0x06U, 0x41U, 0x42U, 0x43U, 0xC6U, 0xD0U, 0x00U
    };
    fwb_framing_cfg_t cfg_beef = {.link_id = 0xBEEFU};
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;

    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_beef, 0x20U, 1U,
                           payload, sizeof(payload),
                           wire, sizeof(wire), &n));
    TEST_ASSERT_EQUAL_UINT(sizeof(expected_wire), n);
    TEST_ASSERT_EQUAL_MEMORY(expected_wire, wire, n);
}

/* fwb_framing_encoded_size: returns 0 for oversized payload. */
static void test_encoded_size_oversized(void)
{
    TEST_ASSERT_EQUAL_UINT(0U,
        fwb_framing_encoded_size((uint16_t)(FWB_FRAMING_MAX_PAYLOAD + 1U)));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_encode_v002);
    RUN_TEST(test_encode_v001_empty_payload);
    RUN_TEST(test_encode_seq_unchanged);
    RUN_TEST(test_encode_payload_too_large);
    RUN_TEST(test_encode_buffer_too_small);
    RUN_TEST(test_encode_null_payload_nonzero_len);
    RUN_TEST(test_encode_payload_not_modified);
    RUN_TEST(test_encode_no_zero_before_delimiter);
    RUN_TEST(test_encode_link_id);
    RUN_TEST(test_encoded_size_oversized);
    return UNITY_END();
}
