#ifndef FWB_CRC_INTERNAL_H
#define FWB_CRC_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

#include "fwb/crc/crc.h"

/* Single place to decide which widths are accepted
 * (currently 8..32; the spec says 8, 16 and 32 only). */
static inline bool fwb_crc_width_valid(uint8_t width)
{
    return width >= 8U && width <= 32U;
}

/* Mask of the low `width` bits. Valid for width 1..32 (no shift by 32). */
static inline uint32_t fwb_crc_mask(uint8_t width)
{
    return UINT32_MAX >> (32U - width);
}

/* Reverse the order of the low `width` bits of `value`. */
static inline uint32_t fwb_crc_reflect(uint32_t value, uint8_t width)
{
    uint32_t result = 0U;

    for (uint8_t i = 0U; i < width; i++) {
        result = (result << 1U) | (value & 1U);
        value >>= 1U;
    }

    return result;
}

/* Register helpers. When refin is set, the register is kept in reflected form
 * from init to finalize. */
uint32_t fwb_crc_state_init(const fwb_crc_params_t *params);
uint32_t fwb_crc_state_finalize(uint32_t reg, const fwb_crc_params_t *params);

/* Implementation selected at build time (CRC-REQ-041). */
uint32_t fwb_crc_state_update_bitwise(uint32_t reg, const uint8_t *data, size_t len,
                                      const fwb_crc_params_t *params);

uint32_t fwb_crc_state_update_table(uint32_t reg, const uint8_t *data, size_t len,
                                    const fwb_crc_table_t *table);

#endif /* FWB_CRC_INTERNAL_H */
