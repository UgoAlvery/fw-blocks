/* FRM-REQ-103: single-bit flip in any position of a valid frame → rejection. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/framing/framing.h"
#include "unity.h"

void setUp(void)  {}
void tearDown(void) {}

static fwb_framing_cfg_t cfg_zero = {.link_id = 0x0000U};

static void flip_and_reject(const uint8_t *wire, size_t wire_len,
                            size_t byte_idx, uint8_t bit_mask)
{
    uint8_t buf[FWB_FRAMING_MAX_WIRE_SIZE];
    memcpy(buf, wire, wire_len);
    buf[byte_idx] ^= bit_mask;

    /* Submit through rx (wire includes delimiter). */
    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    (void)fwb_framing_rx_init(&rx, &cfg_zero, 0U);

    bool got_frame = false;
    for (size_t i = 0U; i < wire_len; i++) {
        bool r = false;
        (void)fwb_framing_rx_push(&rx, buf[i], &frame, &r);
        if (r) { got_frame = true; }
    }

    /* A single-bit flip must not deliver a valid frame. */
    TEST_ASSERT_FALSE_MESSAGE(got_frame, "frame accepted despite bit flip");
}

/* FRM-REQ-103: test on FRM-V-002 reference frame. */
static void test_bitflip_v002(void)
{
    static const uint8_t payload[] = {0x41U, 0x42U, 0x43U};
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x20U, 1U,
                           payload, sizeof(payload), wire, sizeof(wire), &n));

    /* Flip every bit in every byte except the delimiter (last byte). */
    for (size_t byte = 0U; byte < n - 1U; byte++) {
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            flip_and_reject(wire, n, byte, (uint8_t)(1U << bit));
        }
    }
}

/* FRM-REQ-103: test on a frame with zero bytes in payload (COBS boundary). */
static void test_bitflip_zeros_payload(void)
{
    static const uint8_t zeros[4] = {0};
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x01U, 0U,
                           zeros, sizeof(zeros), wire, sizeof(wire), &n));

    for (size_t byte = 0U; byte < n - 1U; byte++) {
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            flip_and_reject(wire, n, byte, (uint8_t)(1U << bit));
        }
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_bitflip_v002);
    RUN_TEST(test_bitflip_zeros_payload);
    return UNITY_END();
}
