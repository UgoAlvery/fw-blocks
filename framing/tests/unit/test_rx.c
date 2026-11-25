#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/framing/framing.h"
#include "unity.h"

void setUp(void)  {}
void tearDown(void) {}

static fwb_framing_cfg_t cfg_zero = {.link_id = 0x0000U};

/* Push a byte array through rx, return status of the last significant event. */
static fwb_status_t push_bytes(fwb_framing_rx_t *rx, const uint8_t *data,
                               size_t len, fwb_framing_frame_t *out,
                               bool *ready)
{
    fwb_status_t last = FWB_OK;
    *ready = false;
    for (size_t i = 0U; i < len; i++) {
        bool r = false;
        fwb_status_t st = fwb_framing_rx_push(rx, data[i], out, &r);
        if (r) {
            *ready = true;
            last   = st;
        } else if (st != FWB_OK) {
            last = st;
        }
    }
    return last;
}

/* FRM-REQ-080: octet par octet, trame complète. */
static void test_rx_basic_frame(void)
{
    static const uint8_t payload[] = {0x41U, 0x42U, 0x43U};
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x20U, 1U,
                           payload, sizeof(payload), wire, sizeof(wire), &n));

    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 0U));

    fwb_status_t st = push_bytes(&rx, wire, n, &frame, &ready);
    TEST_ASSERT_EQUAL_INT(FWB_OK, st);
    TEST_ASSERT_TRUE(ready);
    TEST_ASSERT_EQUAL_HEX8(0x20U, frame.type);
    TEST_ASSERT_EQUAL_UINT(1U, frame.seq);
    TEST_ASSERT_EQUAL_UINT(sizeof(payload), frame.payload_len);
    TEST_ASSERT_EQUAL_MEMORY(payload, frame.payload, sizeof(payload));
}

/* FRM-REQ-082: délimiteur isolé → FWB_OK, pas de livraison. */
static void test_rx_empty_frame_ignored(void)
{
    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 0U));

    fwb_status_t st = fwb_framing_rx_push(&rx, 0x00U, &frame, &ready);
    TEST_ASSERT_EQUAL_INT(FWB_OK, st);
    TEST_ASSERT_FALSE(ready);
}

/* FRM-REQ-083: overflow → FWB_ERR_OVERFLOW. */
static void test_rx_overflow(void)
{
    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 0U));

    fwb_status_t st = FWB_OK;
    /* Fill beyond FWB_FRAMING_MAX_WIRE_SIZE - 1 without a delimiter. */
    for (size_t i = 0U; i < FWB_FRAMING_MAX_WIRE_SIZE; i++) {
        st = fwb_framing_rx_push(&rx, 0x01U, &frame, &ready);
        if (st == FWB_ERR_OVERFLOW) {
            break;
        }
    }
    TEST_ASSERT_EQUAL_INT(FWB_ERR_OVERFLOW, st);
    TEST_ASSERT_FALSE(ready);
}

/* FRM-REQ-084: after overflow, bytes discarded until 0x00. */
static void test_rx_discard_after_overflow(void)
{
    static const uint8_t payload[] = {0x41U, 0x42U};
    uint8_t good_wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x01U, 5U,
                           payload, sizeof(payload),
                           good_wire, sizeof(good_wire), &n));

    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 0U));

    /* Trigger overflow. */
    for (size_t i = 0U; i < FWB_FRAMING_MAX_WIRE_SIZE; i++) {
        fwb_framing_rx_push(&rx, 0x01U, &frame, &ready);
    }
    /* Delimiter ends the discard state. */
    fwb_framing_rx_push(&rx, 0x00U, &frame, &ready);

    /* FRM-REQ-085: next valid frame must succeed. */
    fwb_status_t st = push_bytes(&rx, good_wire, n, &frame, &ready);
    TEST_ASSERT_EQUAL_INT(FWB_OK, st);
    TEST_ASSERT_TRUE(ready);
    TEST_ASSERT_EQUAL_UINT(5U, frame.seq);
}

/* FRM-REQ-085, FRM-REQ-090: after bad CRC, next valid frame accepted; each rejection has a distinct code. */
static void test_rx_recover_after_bad_crc(void)
{
    /* Corrupt wire: flip last CRC byte of a valid frame. */
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    static const uint8_t payload[] = {0x01U};
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x01U, 0U,
                           payload, sizeof(payload),
                           wire, sizeof(wire), &n));
    wire[n - 2U] ^= 0x01U; /* corrupt CRC byte (before delimiter) */

    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 0U));

    /* Bad frame. */
    fwb_status_t st = push_bytes(&rx, wire, n, &frame, &ready);
    TEST_ASSERT_NOT_EQUAL(FWB_OK, st);
    TEST_ASSERT_FALSE(ready);

    /* Good frame immediately after. */
    wire[n - 2U] ^= 0x01U; /* restore CRC */
    st = push_bytes(&rx, wire, n, &frame, &ready);
    TEST_ASSERT_EQUAL_INT(FWB_OK, st);
    TEST_ASSERT_TRUE(ready);
}

/* FRM-REQ-087: timeout abandons frame in progress. */
static void test_rx_timeout(void)
{
    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 3U));

    /* Start receiving. */
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_push(&rx, 0x02U, &frame, &ready));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_push(&rx, 0xAAU, &frame, &ready));

    /* 2 ticks: not yet expired. */
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_tick(&rx));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_tick(&rx));

    /* 3rd tick: timeout. */
    TEST_ASSERT_EQUAL_INT(FWB_ERR_TIMEOUT, fwb_framing_rx_tick(&rx));
    TEST_ASSERT_FALSE(ready);
}

/* FRM-REQ-088: timeout in IDLE → FWB_OK. */
static void test_rx_tick_in_idle(void)
{
    fwb_framing_rx_t rx;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 1U));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_tick(&rx));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_tick(&rx));
}

/* FRM-REQ-089: reset restores IDLE state. */
static void test_rx_reset(void)
{
    fwb_framing_rx_t    rx;
    fwb_framing_frame_t frame;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx, &cfg_zero, 0U));

    /* Start receiving garbage. */
    fwb_framing_rx_push(&rx, 0xAAU, &frame, &ready);
    fwb_framing_rx_push(&rx, 0xBBU, &frame, &ready);

    fwb_framing_rx_reset(&rx);

    /* After reset, idle: empty delimiter ignored. */
    fwb_status_t st = fwb_framing_rx_push(&rx, 0x00U, &frame, &ready);
    TEST_ASSERT_EQUAL_INT(FWB_OK, st);
    TEST_ASSERT_FALSE(ready);
}

/* FRM-REQ-081: push_buf produces same result as byte-by-byte. */
static void test_rx_push_buf_equiv(void)
{
    static const uint8_t payload[] = {0x10U, 0x20U, 0x30U};
    uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t  n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_OK,
        fwb_framing_encode(&cfg_zero, 0x05U, 7U,
                           payload, sizeof(payload), wire, sizeof(wire), &n));

    fwb_framing_rx_t    rx1, rx2;
    fwb_framing_frame_t f1, f2;
    bool r1 = false, r2 = false;
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx1, &cfg_zero, 0U));
    TEST_ASSERT_EQUAL_INT(FWB_OK, fwb_framing_rx_init(&rx2, &cfg_zero, 0U));

    /* Byte-by-byte. */
    push_bytes(&rx1, wire, n, &f1, &r1);

    /* push_buf. */
    size_t consumed = 0U;
    fwb_framing_rx_push_buf(&rx2, wire, n, &consumed, &f2, &r2);

    TEST_ASSERT_EQUAL(r1, r2);
    if (r1 && r2) {
        TEST_ASSERT_EQUAL_HEX8(f1.type, f2.type);
        TEST_ASSERT_EQUAL_UINT(f1.seq, f2.seq);
        TEST_ASSERT_EQUAL_UINT(f1.payload_len, f2.payload_len);
        TEST_ASSERT_EQUAL_MEMORY(f1.payload, f2.payload, f1.payload_len);
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_rx_basic_frame);
    RUN_TEST(test_rx_empty_frame_ignored);
    RUN_TEST(test_rx_overflow);
    RUN_TEST(test_rx_discard_after_overflow);
    RUN_TEST(test_rx_recover_after_bad_crc);
    RUN_TEST(test_rx_timeout);
    RUN_TEST(test_rx_tick_in_idle);
    RUN_TEST(test_rx_reset);
    RUN_TEST(test_rx_push_buf_equiv);
    return UNITY_END();
}
