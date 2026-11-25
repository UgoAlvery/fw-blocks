/* libFuzzer harness — FRM-REQ-081
 * Verifies that byte-by-byte and push_buf produce the same first frame
 * regardless of how the input stream is split. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/framing/framing.h"

static fwb_framing_cfg_t k_cfg = {.link_id = 0U};

// NOLINTNEXTLINE(readability-identifier-naming)
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

// NOLINTNEXTLINE(readability-identifier-naming)
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2U) {
        return 0;
    }

    /* First byte controls chunk size: 1..32. */
    size_t chunk = (size_t)(data[0] & 0x1FU) + 1U;
    const uint8_t *stream = data + 1U;
    size_t slen = size - 1U;

    static fwb_framing_rx_t rx1;
    static fwb_framing_rx_t rx2;
    static fwb_framing_frame_t f1;
    static fwb_framing_frame_t f2;

    (void)fwb_framing_rx_init(&rx1, &k_cfg, 0U);
    (void)fwb_framing_rx_init(&rx2, &k_cfg, 0U);

    /* rx1: byte-by-byte */
    bool ready1 = false;
    fwb_status_t last1 = FWB_OK;
    for (size_t i = 0U; i < slen && !ready1; i++) {
        last1 = fwb_framing_rx_push(&rx1, stream[i], &f1, &ready1);
    }

    /* rx2: push_buf with chunk-sized slices */
    bool ready2 = false;
    fwb_status_t last2 = FWB_OK;
    size_t pos = 0U;
    while (pos < slen && !ready2) {
        size_t n = slen - pos;
        if (n > chunk) {
            n = chunk;
        }
        size_t consumed = 0U;
        last2 = fwb_framing_rx_push_buf(&rx2, stream + pos, n, &consumed, &f2, &ready2);
        pos += (consumed > 0U) ? consumed : n;
    }

    /* FRM-REQ-081: both paths must agree on the first frame. */
    if (ready1 != ready2) {
        __builtin_trap();
    }
    if (ready1) {
        if (last1 != last2) {
            __builtin_trap();
        }
        if (f1.type != f2.type) {
            __builtin_trap();
        }
        if (f1.seq != f2.seq) {
            __builtin_trap();
        }
        if (f1.payload_len != f2.payload_len) {
            __builtin_trap();
        }
        if (f1.payload_len > 0U && memcmp(f1.payload, f2.payload, f1.payload_len) != 0) {
            __builtin_trap();
        }
    }

    return 0;
}
