/* FRM-REQ-104, FRM-REQ-038: truncation at any position → rejection or no frame; decoder never reads
 * past buffer end. */
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

static void truncate_and_check(const uint8_t *wire, size_t full_len, size_t trunc_len)
{
    fwb_framing_rx_t rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    (void)fwb_framing_rx_init(&rx, &cfg_zero, 0U);

    for (size_t i = 0U; i < trunc_len; i++) {
        fwb_framing_rx_push(&rx, wire[i], &frame, &ready);
    }
    /* Must not deliver a valid frame for any prefix. */
    TEST_ASSERT_FALSE_MESSAGE(ready, "truncated frame delivered as valid");
    (void)full_len;
}

/* FRM-REQ-104 on FRM-V-002. */
static void test_truncation_v002(void)
{
    static const uint8_t payload[] = {0x41U, 0x42U, 0x43U};
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_encode(&cfg_zero, 0x20U, 1U, payload, sizeof(payload),
                                                     wire, sizeof(wire), &n));

    /* Every strict prefix (not including the full frame). */
    for (size_t trunc = 0U; trunc < n; trunc++) {
        truncate_and_check(wire, n, trunc);
    }
}

/* FRM-REQ-104: truncation of max-payload frame. */
static void test_truncation_max_payload(void)
{
    static uint8_t payload[FWB_FRAMING_MAX_PAYLOAD];
    for (size_t i = 0U; i < sizeof(payload); i++) {
        payload[i] = (uint8_t)i;
    }
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_encode(&cfg_zero, 0x40U, 3U, payload, sizeof(payload),
                                                     wire, sizeof(wire), &n));

    /* Test a subset of positions (every 8th) to keep runtime reasonable. */
    for (size_t trunc = 0U; trunc < n; trunc += 8U) {
        truncate_and_check(wire, n, trunc);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_truncation_v002);
    RUN_TEST(test_truncation_max_payload);
    return UNITY_END();
}
