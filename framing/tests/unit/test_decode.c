#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/framing/framing.h"

#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

static fwb_framing_cfg_t cfg_zero = {.link_id = 0x0000U};

/* Convenience: encode then decode, check round-trip. */
static void check_encode_decode(uint8_t type, uint16_t seq, const uint8_t *payload,
                                uint16_t payload_len)
{
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t n = 0U;
    fwb_framing_frame_t out;
    memset(&out, 0xFFU, sizeof(out));

    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_encode(&cfg_zero, type, seq, payload, payload_len,
                                                     wire, sizeof(wire), &n));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_decode_wire(&cfg_zero, wire, n - 1U, &out));
    TEST_ASSERT_EQUAL_HEX8(type, out.type);
    TEST_ASSERT_EQUAL_UINT(seq, out.seq);
    TEST_ASSERT_EQUAL_UINT(payload_len, out.payload_len);
    if (payload_len > 0U) {
        TEST_ASSERT_EQUAL_MEMORY(payload, out.payload, payload_len);
    }
}

/* FRM-REQ-040, FRM-REQ-041: empty payload accepted */
static void test_decode_empty_payload(void)
{
    check_encode_decode(0x10U, 0U, NULL, 0U);
}

/* FRM-REQ-040: normal payload */
static void test_decode_abc(void)
{
    static const uint8_t payload[] = {0x41U, 0x42U, 0x43U};
    check_encode_decode(0x20U, 1U, payload, sizeof(payload));
}

/* FRM-REQ-031: frame too short (< 9 bytes decoded) */
static void test_decode_too_short(void)
{
    /* FRM-I-007: 6-byte decoded frame. */
    static const uint8_t frame[] = {0x01U, 0x00U, 0x20U, 0x01U, 0x00U, 0x03U};
    fwb_framing_frame_t out;
    memset(&out, 0xFFU, sizeof(out));
    TEST_ASSERT_EQUAL_INT(FWB_ERR_FRAME_TOO_SHORT,
                          fwb_framing_decode(&cfg_zero, frame, sizeof(frame), &out));
    /* FRM-REQ-039: out must not have been touched. */
    uint8_t sentinel[sizeof(out)];
    memset(sentinel, 0xFFU, sizeof(sentinel));
    TEST_ASSERT_EQUAL_MEMORY(sentinel, &out, sizeof(out));
}

/* FRM-REQ-032: LEN > MAX_PAYLOAD */
static void test_decode_payload_too_large(void)
{
    /* FRM-I-006: LEN=0x0101=257. */
    static const uint8_t frame[] = {0x01U, 0x00U, 0x20U, 0x01U, 0x00U, 0x01U,
                                    0x01U, 0x41U, 0x42U, 0x43U, 0xC4U, 0xCDU};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_PAYLOAD_TOO_LARGE,
                          fwb_framing_decode(&cfg_zero, frame, sizeof(frame), &out));
}

/* FRM-REQ-033: LEN mismatch */
static void test_decode_length_mismatch(void)
{
    /* FRM-I-003: LEN=4, only 3 bytes present. */
    static const uint8_t frame[] = {0x01U, 0x00U, 0x20U, 0x01U, 0x00U, 0x04U,
                                    0x00U, 0x41U, 0x42U, 0x43U, 0x27U, 0x98U};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_LENGTH_MISMATCH,
                          fwb_framing_decode(&cfg_zero, frame, sizeof(frame), &out));
}

/* FRM-REQ-034: bad CRC */
static void test_decode_bad_crc(void)
{
    /* FRM-I-001: bit 0 of CRC flipped vs FRM-V-002. */
    static const uint8_t frame[] = {0x01U, 0x00U, 0x20U, 0x01U, 0x00U, 0x03U,
                                    0x00U, 0x41U, 0x42U, 0x43U, 0xF2U, 0xFFU};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_BAD_CRC,
                          fwb_framing_decode(&cfg_zero, frame, sizeof(frame), &out));
}

/* FRM-REQ-035: bad version (checked AFTER CRC — CRC is valid here). */
static void test_decode_bad_version(void)
{
    /* FRM-I-004: VER=0x02, CRC valid. */
    static const uint8_t frame[] = {0x02U, 0x00U, 0x20U, 0x01U, 0x00U, 0x03U,
                                    0x00U, 0x41U, 0x42U, 0x43U, 0x3CU, 0x4EU};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_BAD_VERSION,
                          fwb_framing_decode(&cfg_zero, frame, sizeof(frame), &out));
}

/* FRM-REQ-036: bad flags (checked AFTER CRC — CRC is valid here). */
static void test_decode_bad_flags(void)
{
    /* FRM-I-005: FLAGS=0x01, CRC valid. */
    static const uint8_t frame[] = {0x01U, 0x01U, 0x20U, 0x01U, 0x00U, 0x03U,
                                    0x00U, 0x41U, 0x42U, 0x43U, 0xD0U, 0x14U};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_BAD_FLAGS,
                          fwb_framing_decode(&cfg_zero, frame, sizeof(frame), &out));
}

/* FRM-REQ-037: wrong link_id → BAD_CRC. */
static void test_decode_wrong_link_id(void)
{
    /* FRM-V-006 was encoded with link_id=0xBEEF; decode with 0x0000 → BAD_CRC. */
    static const uint8_t wire_beef[] = {0x02U, 0x01U, 0x03U, 0x20U, 0x01U, 0x02U, 0x03U,
                                        0x06U, 0x41U, 0x42U, 0x43U, 0xC6U, 0xD0U};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_BAD_CRC,
                          fwb_framing_decode_wire(&cfg_zero, wire_beef, sizeof(wire_beef), &out));
}

/* FRM-REQ-042: input frame not modified. */
static void test_decode_frame_not_modified(void)
{
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t n = 0U;
    fwb_framing_frame_t out;
    static const uint8_t payload[] = {0x01U, 0x02U};

    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_encode(&cfg_zero, 0x01U, 0U, payload, sizeof(payload),
                                                     wire, sizeof(wire), &n));

    uint8_t copy[FWB_FRAMING_MAX_WIRE_SIZE];
    memcpy(copy, wire, n - 1U);

    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_decode_wire(&cfg_zero, wire, n - 1U, &out));
    TEST_ASSERT_EQUAL_MEMORY(copy, wire, n - 1U);
}

/* FRM-REQ-030: check that PAYLOAD_TOO_LARGE comes before LENGTH_MISMATCH. */
static void test_decode_check_order_len_vs_mismatch(void)
{
    /* LEN=257 also causes length mismatch, but check 2 must fire first. */
    static const uint8_t frame[] = {0x01U, 0x00U, 0x20U, 0x01U, 0x00U, 0x01U,
                                    0x01U, 0x41U, 0x42U, 0x43U, 0xC4U, 0xCDU};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_PAYLOAD_TOO_LARGE,
                          fwb_framing_decode(&cfg_zero, frame, sizeof(frame), &out));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_decode_empty_payload);
    RUN_TEST(test_decode_abc);
    RUN_TEST(test_decode_too_short);
    RUN_TEST(test_decode_payload_too_large);
    RUN_TEST(test_decode_length_mismatch);
    RUN_TEST(test_decode_bad_crc);
    RUN_TEST(test_decode_bad_version);
    RUN_TEST(test_decode_bad_flags);
    RUN_TEST(test_decode_wrong_link_id);
    RUN_TEST(test_decode_frame_not_modified);
    RUN_TEST(test_decode_check_order_len_vs_mismatch);
    return UNITY_END();
}
