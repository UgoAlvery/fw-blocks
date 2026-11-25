/* libFuzzer harness — FRM-REQ-060, FRM-REQ-061, FRM-REQ-066
 * Tests COBS properties on arbitrary payload bytes via the public framing API:
 * no 0x00 before delimiter, round-trip identity, overhead within bound. */
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
    /* First 3 bytes: type(1) + seq(2 LE); rest is payload. */
    if (size < 3U) {
        return 0;
    }

    uint8_t type = data[0];
    uint16_t seq = (uint16_t)((uint16_t)data[1] | ((uint16_t)data[2] << 8));
    const uint8_t *payload = data + 3U;
    size_t payload_len = size - 3U;

    if (payload_len > FWB_FRAMING_MAX_PAYLOAD) {
        payload_len = FWB_FRAMING_MAX_PAYLOAD;
    }

    static uint8_t wire[FWB_FRAMING_MAX_WIRE_SIZE];
    size_t n = 0U;

    if (fwb_framing_encode(&k_cfg, type, seq, payload, (uint16_t)payload_len, wire, sizeof(wire),
                           &n) != FWB_OK) {
        return 0;
    }

    /* FRM-REQ-066: wire size must be within computed bound.
     * Frame decoded = 7+LEN+2 = 9+LEN bytes; COBS overhead <= 1+floor(x/254). */
    size_t raw_len = 9U + payload_len;
    size_t max_wire = raw_len + 1U + raw_len / 254U + 1U; /* +1 for delimiter */
    if (n > max_wire) {
        __builtin_trap();
    }

    /* FRM-REQ-060: no 0x00 before the final delimiter. */
    for (size_t i = 0U; i + 1U < n; i++) {
        if (wire[i] == 0x00U) {
            __builtin_trap();
        }
    }
    if (n == 0U || wire[n - 1U] != 0x00U) {
        __builtin_trap();
    }

    /* FRM-REQ-061: decode must reproduce exact type, seq and payload. */
    fwb_framing_frame_t out;
    if (fwb_framing_decode_wire(&k_cfg, wire, n - 1U, &out) != FWB_OK) {
        __builtin_trap();
    }
    if (out.type != type) {
        __builtin_trap();
    }
    if (out.seq != seq) {
        __builtin_trap();
    }
    if (out.payload_len != (uint16_t)payload_len) {
        __builtin_trap();
    }
    if (payload_len > 0U && memcmp(out.payload, payload, payload_len) != 0) {
        __builtin_trap();
    }

    return 0;
}
