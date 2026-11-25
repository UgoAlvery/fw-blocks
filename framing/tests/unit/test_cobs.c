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

/* Helper: encode then decode, check round-trip. */
static void check_roundtrip(const uint8_t *in, size_t in_len)
{
    /* Use decode_wire path via a fresh encode to exercise both sides. */
    static uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    static uint8_t decoded[FWB_FRAMING_MAX_FRAME_SIZE];

    /* We test COBS directly via encode/decode_wire with a valid frame, but
     * for raw COBS we need the internal symbols. Access them indirectly:
     * encode a payload of arbitrary bytes through fwb_framing_encode by
     * wrapping in a valid frame is too indirect. Instead we just test via
     * fwb_framing_decode_wire on hand-crafted COBS data for specific cases,
     * and trust the vector tests for integration. */

    /* For the round-trip test, use encode→wire→decode_wire on real frames. */
    fwb_framing_cfg_t cfg = {.link_id = 0x0000U};
    size_t n = 0U;
    fwb_framing_frame_t frame;

    if (in_len > FWB_FRAMING_MAX_PAYLOAD) {
        return; /* skip oversized */
    }

    fwb_status_t st = fwb_framing_encode(&cfg, 0x42U, 0U, in_len > 0U ? in : NULL, (uint16_t)in_len,
                                         wire, sizeof(wire), &n);
    TEST_ASSERT_EQUAL_INT(FWB_OK, st);

    /* Verify no 0x00 before delimiter (FRM-REQ-021, FRM-REQ-060). */
    for (size_t i = 0U; i + 1U < n; i++) {
        TEST_ASSERT_NOT_EQUAL_MESSAGE(0U, wire[i], "zero before delimiter");
    }
    TEST_ASSERT_EQUAL_HEX8(0x00U, wire[n - 1U]);

    /* Decode back (excludes delimiter). */
    st = fwb_framing_decode_wire(&cfg, wire, n - 1U, &frame);
    TEST_ASSERT_EQUAL_INT(FWB_OK, st);
    TEST_ASSERT_EQUAL_UINT(in_len, frame.payload_len);
    if (in_len > 0U) {
        TEST_ASSERT_EQUAL_MEMORY(in, frame.payload, in_len);
    }

    /* FRM-REQ-066: wire overhead <= 1 + floor(frame_len/254) + 1 (delimiter). */
    size_t frame_len = FWB_FRAMING_HEADER_SIZE + in_len + FWB_FRAMING_CRC_SIZE;
    size_t max_wire = frame_len + 1U + (frame_len / 254U) + 1U;
    TEST_ASSERT_TRUE_MESSAGE(n <= max_wire, "COBS overhead exceeded");

    (void)decoded;
}

/* FRM-REQ-061: round-trip, empty payload */
static void test_roundtrip_empty(void)
{
    check_roundtrip(NULL, 0U);
}

/* FRM-REQ-061: round-trip, all zeros payload */
static void test_roundtrip_all_zeros(void)
{
    static const uint8_t zeros[16] = {0};
    check_roundtrip(zeros, sizeof(zeros));
}

/* FRM-REQ-062: 254-byte block of non-zero bytes → code 0xFF */
static void test_roundtrip_254_nonzero(void)
{
    static uint8_t buf[254];
    memset(buf, 0xAAU, sizeof(buf));
    check_roundtrip(buf, sizeof(buf));
}

/* FRM-REQ-062: more than 254 non-zero bytes forces split */
static void test_roundtrip_255_nonzero(void)
{
    static uint8_t buf[255];
    memset(buf, 0x55U, sizeof(buf));
    check_roundtrip(buf, sizeof(buf));
}

/* FRM-REQ-063: payload ending with zero */
static void test_roundtrip_trailing_zero(void)
{
    static const uint8_t data[] = {0x01U, 0x02U, 0x00U};
    check_roundtrip(data, sizeof(data));
}

/* FRM-REQ-060: no zero before delimiter for all-zero payload */
static void test_no_zero_in_cobs(void)
{
    static const uint8_t zeros[8] = {0};
    check_roundtrip(zeros, sizeof(zeros));
}

/* FRM-REQ-064: code 0x00 inside COBS → COBS_MALFORMED */
static void test_decode_wire_null_code(void)
{
    /* Single byte 0x00 in COBS zone (not the delimiter). */
    static const uint8_t bad_wire[] = {0x00U, 0x01U, 0x02U};
    fwb_framing_cfg_t cfg = {0};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_COBS_MALFORMED,
                          fwb_framing_decode_wire(&cfg, bad_wire, sizeof(bad_wire), &out));
}

/* FRM-REQ-065: code pointing past end of buffer → COBS_MALFORMED */
static void test_decode_wire_code_overflow(void)
{
    /* From vector FRM-I-008: code=5 but only 2 bytes follow. */
    static const uint8_t bad_wire[] = {0x05U, 0x01U, 0x02U};
    fwb_framing_cfg_t cfg = {0};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_COBS_MALFORMED,
                          fwb_framing_decode_wire(&cfg, bad_wire, sizeof(bad_wire), &out));
}

/* FRM-REQ-066: COBS overhead check for max payload */
static void test_cobs_overhead_max_payload(void)
{
    static uint8_t payload[FWB_FRAMING_MAX_PAYLOAD];
    for (size_t i = 0U; i < sizeof(payload); i++) {
        payload[i] = (uint8_t)i;
    }
    check_roundtrip(payload, sizeof(payload));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_roundtrip_empty);
    RUN_TEST(test_roundtrip_all_zeros);
    RUN_TEST(test_roundtrip_254_nonzero);
    RUN_TEST(test_roundtrip_255_nonzero);
    RUN_TEST(test_roundtrip_trailing_zero);
    RUN_TEST(test_no_zero_in_cobs);
    RUN_TEST(test_decode_wire_null_code);
    RUN_TEST(test_decode_wire_code_overflow);
    RUN_TEST(test_cobs_overhead_max_payload);
    return UNITY_END();
}
