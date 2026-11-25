#include <stdint.h>

#include "framing_internal.h"

/* FRM-REQ-060, 061, 062, 063, 064, 065, 066 */

size_t framing_cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap)
{
    /* Minimum output: 1 code byte even for empty input. */
    const size_t max_out = in_len + 1U + (in_len / 254U);
    if (out_cap < max_out) {
        return 0U;
    }

    size_t code_pos = 0U;
    size_t out_pos = 1U;
    uint8_t run = 0U;

    for (size_t i = 0U; i < in_len; i++) {
        if (in[i] != 0U) {
            out[out_pos++] = in[i];
            run++;
            if (run == 254U) {
                /* Full block: code 0xFF, no implicit zero follows. */
                out[code_pos] = 0xFFU;
                code_pos = out_pos;
                out[out_pos++] = 0U; /* placeholder for next code */
                run = 0U;
            }
        } else {
            out[code_pos] = (uint8_t)(run + 1U);
            code_pos = out_pos;
            out[out_pos++] = 0U; /* placeholder for next code */
            run = 0U;
        }
    }
    out[code_pos] = (uint8_t)(run + 1U);

    return out_pos;
}

fwb_status_t framing_cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap,
                                 size_t *out_len)
{
    size_t pos = 0U;
    size_t out_pos = 0U;

    while (pos < in_len) {
        uint8_t code = in[pos];
        if (code == 0U) {
            return FWB_ERR_COBS_MALFORMED; /* FRM-REQ-064 */
        }

        size_t next = pos + (size_t)code;
        if (next > in_len) {
            return FWB_ERR_COBS_MALFORMED; /* FRM-REQ-065 */
        }

        /* Copy (code-1) data bytes. */
        for (size_t j = pos + 1U; j < next; j++) {
            if (out_pos >= out_cap) {
                return FWB_ERR_COBS_MALFORMED;
            }
            out[out_pos++] = in[j];
        }

        /* Write implicit zero unless this is a full block or the last block. */
        if (code != 0xFFU && next < in_len) {
            if (out_pos >= out_cap) {
                return FWB_ERR_COBS_MALFORMED;
            }
            out[out_pos++] = 0U;
        }

        pos = next;
    }

    *out_len = out_pos;
    return FWB_OK;
}
