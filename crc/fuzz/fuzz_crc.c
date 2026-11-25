/* libFuzzer harness — CRC-REQ-050, CRC-REQ-051, CRC-REQ-052
 * ASAN+UBSAN catch out-of-bounds and UB; __builtin_trap on cross-impl mismatch
 * catches correctness divergence (also covers unaligned-input correctness). */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/crc/crc.h"

/* Two predefined param sets to exercise; index selected by first fuzz byte. */
static const fwb_crc_params_t *const k_params[2] = {
    &fwb_crc_params_ccitt_false,
    &fwb_crc_params_crc32c,
};

// NOLINTNEXTLINE(readability-identifier-naming)
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

// NOLINTNEXTLINE(readability-identifier-naming)
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 1U) {
        return 0;
    }

    const fwb_crc_params_t *params = k_params[data[0] & 1U];
    const uint8_t *payload = data + 1U;
    size_t payload_len = size - 1U;

    /* Bitwise path. */
    fwb_crc_ctx_t ctx_bit;
    uint32_t result_bit = 0U;
    if (fwb_crc_init(&ctx_bit, params) != FWB_OK) {
        return 0;
    }
    if (fwb_crc_update(&ctx_bit, payload, payload_len) != FWB_OK) {
        return 0;
    }
    if (fwb_crc_finalize(&ctx_bit, &result_bit) != FWB_OK) {
        return 0;
    }

    /* Table-driven path — CRC-REQ-052: same result regardless of alignment. */
    fwb_crc_ctx_t ctx_tbl;
    fwb_crc_table_t tbl;
    uint32_t result_tbl = 0U;
    if (fwb_crc_table_build(params, &tbl) != FWB_OK) {
        return 0;
    }
    if (fwb_crc_init(&ctx_tbl, params) != FWB_OK) {
        return 0;
    }
    if (fwb_crc_update_table(&ctx_tbl, &tbl, payload, payload_len) != FWB_OK) {
        return 0;
    }
    if (fwb_crc_finalize(&ctx_tbl, &result_tbl) != FWB_OK) {
        return 0;
    }

    /* Cross-check: both paths must agree on every input. */
    if (result_bit != result_tbl) {
        __builtin_trap();
    }

    /* Oneshot must also agree. */
    uint32_t result_one = 0U;
    if (fwb_crc_oneshot(params, payload, payload_len, &result_one) != FWB_OK) {
        return 0;
    }
    if (result_one != result_bit) {
        __builtin_trap();
    }

    return 0;
}
