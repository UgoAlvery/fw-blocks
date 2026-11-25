#include <stddef.h>
#include <stdint.h>

#include "fwb/framing/framing.h"

#include "unity.h"

void setUp(void)
{
}
void tearDown(void)
{
}

/* FRM-REQ-004 */
static void test_constants_usable_in_array_decl(void)
{
    uint8_t wire_buf[FWB_FRAMING_MAX_WIRE_SIZE] = {0};
    uint8_t frame_buf[FWB_FRAMING_MAX_FRAME_SIZE] = {0};
    (void)wire_buf;
    (void)frame_buf;
    TEST_ASSERT_EQUAL_UINT(268U, FWB_FRAMING_MAX_WIRE_SIZE);
    TEST_ASSERT_EQUAL_UINT(265U, FWB_FRAMING_MAX_FRAME_SIZE);
    TEST_ASSERT_EQUAL_UINT(9U, FWB_FRAMING_MIN_FRAME_SIZE);
    TEST_ASSERT_EQUAL_UINT(1U, FWB_FRAMING_VERSION);
}

/* FRM-REQ-005 */
static void test_max_payload_default(void)
{
    TEST_ASSERT_EQUAL_UINT(256U, FWB_FRAMING_MAX_PAYLOAD);
}

/* FRM-REQ-003: null cfg */
static void test_encode_null_cfg(void)
{
    uint8_t dst[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
                          fwb_framing_encode(NULL, 0U, 0U, NULL, 0U, dst, sizeof(dst), &n));
}

/* FRM-REQ-003: null dst */
static void test_encode_null_dst(void)
{
    fwb_framing_cfg_t cfg = {0};
    size_t n = 0U;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
                          fwb_framing_encode(&cfg, 0U, 0U, NULL, 0U, NULL, 0U, &n));
}

/* FRM-REQ-003: null out_len */
static void test_encode_null_out_len(void)
{
    fwb_framing_cfg_t cfg = {0};
    uint8_t dst[FWB_FRAMING_MAX_WIRE_SIZE];
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
                          fwb_framing_encode(&cfg, 0U, 0U, NULL, 0U, dst, sizeof(dst), NULL));
}

/* FRM-REQ-003: null cfg for decode */
static void test_decode_null_cfg(void)
{
    uint8_t frame[9] = {0};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
                          fwb_framing_decode(NULL, frame, sizeof(frame), &out));
}

/* FRM-REQ-003: null frame for decode */
static void test_decode_null_frame(void)
{
    fwb_framing_cfg_t cfg = {0};
    fwb_framing_frame_t out;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_framing_decode(&cfg, NULL, 0U, &out));
}

/* FRM-REQ-003: null out for decode */
static void test_decode_null_out(void)
{
    fwb_framing_cfg_t cfg = {0};
    uint8_t frame[9] = {0};
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG,
                          fwb_framing_decode(&cfg, frame, sizeof(frame), NULL));
}

/* FRM-REQ-003: null rx */
static void test_rx_init_null(void)
{
    fwb_framing_cfg_t cfg = {0};
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_framing_rx_init(NULL, &cfg, 0U));
}

/* FRM-REQ-003: null cfg for rx */
static void test_rx_init_null_cfg(void)
{
    fwb_framing_rx_t rx;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_framing_rx_init(&rx, NULL, 0U));
}

/* FRM-REQ-003: null rx_push */
static void test_rx_push_null_rx(void)
{
    fwb_framing_frame_t out;
    bool ready = false;
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_framing_rx_push(NULL, 0U, &out, &ready));
}

/* FRM-REQ-003: null out for rx_push */
static void test_rx_push_null_out(void)
{
    fwb_framing_rx_t rx;
    fwb_framing_cfg_t cfg = {0};
    bool ready = false;
    (void)fwb_framing_rx_init(&rx, &cfg, 0U);
    TEST_ASSERT_EQUAL_INT(FWB_ERR_INVALID_ARG, fwb_framing_rx_push(&rx, 0U, NULL, &ready));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_constants_usable_in_array_decl);
    RUN_TEST(test_max_payload_default);
    RUN_TEST(test_encode_null_cfg);
    RUN_TEST(test_encode_null_dst);
    RUN_TEST(test_encode_null_out_len);
    RUN_TEST(test_decode_null_cfg);
    RUN_TEST(test_decode_null_frame);
    RUN_TEST(test_decode_null_out);
    RUN_TEST(test_rx_init_null);
    RUN_TEST(test_rx_init_null_cfg);
    RUN_TEST(test_rx_push_null_rx);
    RUN_TEST(test_rx_push_null_out);
    return UNITY_END();
}
