#ifndef FWB_FRAMING_INTERNAL_H
#define FWB_FRAMING_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "fwb/common/status.h"

/* COBS encode: in[0..in_len-1] → out buffer.
 * out_cap must be >= in_len + 1 + in_len/254.
 * Returns bytes written, or 0 if out_cap is insufficient. */
size_t framing_cobs_encode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap);

/* COBS decode: in[0..in_len-1] → out buffer (must be >= in_len bytes).
 * Returns FWB_OK and sets *out_len, or FWB_ERR_COBS_MALFORMED. */
fwb_status_t framing_cobs_decode(const uint8_t *in, size_t in_len, uint8_t *out, size_t out_cap,
                                 size_t *out_len);

#endif /* FWB_FRAMING_INTERNAL_H */
