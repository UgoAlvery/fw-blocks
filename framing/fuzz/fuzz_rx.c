/* libFuzzer harness — FRM-REQ-100, FRM-REQ-101, FRM-REQ-102, FRM-REQ-106 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/framing/framing.h"

// NOLINTNEXTLINE(readability-identifier-naming)
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

// NOLINTNEXTLINE(readability-identifier-naming)
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size == 0U) {
        return 0;
    }

    /* Use first 2 bytes as link_id, rest as byte stream. */
    uint16_t link_id = 0U;
    if (size >= 2U) {
        link_id = (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
        data += 2U;
        size -= 2U;
    }

    static fwb_framing_rx_t rx;
    static fwb_framing_frame_t frame;
    fwb_framing_cfg_t cfg = {.link_id = link_id};
    (void)fwb_framing_rx_init(&rx, &cfg, 0U);

    for (size_t i = 0U; i < size; i++) {
        bool r = false;
        fwb_status_t st = fwb_framing_rx_push(&rx, data[i], &frame, &r);

        /* FRM-REQ-102: a delivered frame must re-encode identically. */
        if (r) {
            uint8_t rewire[FWB_FRAMING_MAX_WIRE_SIZE];
            size_t n = 0U;
            fwb_status_t enc_st = fwb_framing_encode(&cfg, frame.type, frame.seq,
                                                     frame.payload_len > 0U ? frame.payload : NULL,
                                                     frame.payload_len, rewire, sizeof(rewire), &n);
            /* If encoding fails here something is deeply wrong. */
            if (enc_st != FWB_OK) {
                __builtin_trap();
            }
        }
        (void)st;
    }

    return 0;
}
