#include "crc_internal.h"

/* Same bit order as the register: reflected (right shift) when refin. */
static uint32_t fwb_crc_table_entry(uint32_t index, const fwb_crc_params_t *params)
{
    const uint32_t mask = fwb_crc_mask(params->width);
    uint32_t value;

    if (params->refin) {
        const uint32_t reflected_poly = fwb_crc_reflect(params->poly, params->width);

        value = index;
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            value = ((value & 1U) != 0U) ? ((value >> 1U) ^ reflected_poly) : (value >> 1U);
        }
    } else {
        const uint32_t topbit = UINT32_C(1) << (params->width - 1U);

        value = index << (params->width - 8U);
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            value = ((value & topbit) != 0U) ? ((value << 1U) ^ params->poly) : (value << 1U);
            value &= mask;
        }
    }

    return value & mask;
}

fwb_status_t fwb_crc_table_build(const fwb_crc_params_t *params, fwb_crc_table_t *table)
{
    if (params == NULL || table == NULL || !fwb_crc_width_valid(params->width)) {
        return FWB_ERR_INVALID_ARG;
    }

    for (uint32_t i = 0U; i < 256U; i++) {
        table->entries[i] = fwb_crc_table_entry(i, params);
    }
    table->params = *params;

    return FWB_OK;
}

uint32_t fwb_crc_state_update_table(uint32_t reg, const uint8_t *data, size_t len,
                                    const fwb_crc_table_t *table)
{
    if (table == NULL || !fwb_crc_width_valid(table->params.width) || (data == NULL && len != 0U)) {
        return reg;
    }

    const uint32_t mask = fwb_crc_mask(table->params.width);
    uint32_t crc = reg;

    if (table->params.refin) {
        for (size_t i = 0U; i < len; i++) {
            crc = (crc >> 8U) ^ table->entries[(crc ^ (uint32_t)data[i]) & 0xFFU];
        }
    } else {
        const uint32_t shift = table->params.width - 8U;

        for (size_t i = 0U; i < len; i++) {
            const uint32_t index = ((crc >> shift) ^ (uint32_t)data[i]) & 0xFFU;

            crc = ((crc << 8U) ^ table->entries[index]) & mask;
        }
    }

    return crc;
}

fwb_status_t fwb_crc_update_table(fwb_crc_ctx_t *ctx, const fwb_crc_table_t *table,
                                  const uint8_t *data, size_t len)
{
    if (ctx == NULL || table == NULL || (data == NULL && len != 0U)) {
        return FWB_ERR_INVALID_ARG;
    }

    /* Precondition: ctx initialized by fwb_crc_init(). */
    if (!fwb_crc_width_valid(ctx->params.width)) {
        return FWB_ERR_INVALID_ARG;
    }

    /* The table must belong to the same parameter set as the context. */
    if (table->params.width != ctx->params.width || table->params.poly != ctx->params.poly ||
        table->params.refin != ctx->params.refin) {
        return FWB_ERR_INVALID_ARG;
    }

    ctx->reg = fwb_crc_state_update_table(ctx->reg, data, len, table);

    return FWB_OK;
}
