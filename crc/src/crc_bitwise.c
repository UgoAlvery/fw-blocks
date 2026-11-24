#include "crc_internal.h"

uint32_t fwb_crc_state_update_bitwise(uint32_t reg, const uint8_t *data, size_t len,
                                      const fwb_crc_params_t *params)
{
    if (params == NULL || !fwb_crc_width_valid(params->width) || (data == NULL && len != 0U)) {
        return reg;
    }

    const uint32_t mask = fwb_crc_mask(params->width);
    uint32_t crc = reg;

    if (params->refin) {
        const uint32_t reflected_poly = fwb_crc_reflect(params->poly, params->width);

        for (size_t i = 0U; i < len; i++) {
            crc ^= (uint32_t)data[i];

            for (uint8_t bit = 0U; bit < 8U; bit++) {
                if ((crc & 1U) != 0U) {
                    crc = (crc >> 1U) ^ reflected_poly;
                } else {
                    crc >>= 1U;
                }

                crc &= mask;
            }
        }
    } else {
        const uint32_t topbit = UINT32_C(1) << (params->width - 1U);
        const uint32_t shift = params->width - 8U;

        for (size_t i = 0U; i < len; i++) {
            crc ^= (uint32_t)data[i] << shift;

            for (uint8_t bit = 0U; bit < 8U; bit++) {
                if ((crc & topbit) != 0U) {
                    crc = (crc << 1U) ^ params->poly;
                } else {
                    crc <<= 1U;
                }

                crc &= mask;
            }
        }
    }

    return crc;
}
