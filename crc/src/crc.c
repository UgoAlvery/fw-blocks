#include "crc_internal.h"

uint32_t fwb_crc_state_init(const fwb_crc_params_t *params)
{
    if (params == NULL || !fwb_crc_width_valid(params->width)) {
        return 0U;
    }

    const uint32_t init = params->init & fwb_crc_mask(params->width);

    return params->refin ? fwb_crc_reflect(init, params->width) : init;
}

uint32_t fwb_crc_state_finalize(uint32_t reg, const fwb_crc_params_t *params)
{
    if (params == NULL || !fwb_crc_width_valid(params->width)) {
        return 0U;
    }

    uint32_t out = reg;

    if (params->refout != params->refin) {
        out = fwb_crc_reflect(out, params->width);
    }

    return (out ^ params->xorout) & fwb_crc_mask(params->width);
}

/* TODO: public API (fwb_crc_init / update / finalize / oneshot) goes here. */

fwb_status_t fwb_crc_init(fwb_crc_ctx_t *ctx, const fwb_crc_params_t *params)
{
    if (ctx == NULL || params == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    if (!fwb_crc_width_valid(params->width)) {
        return FWB_ERR_INVALID_ARG;
    }

    ctx->params = *params;
    ctx->reg = fwb_crc_state_init(params);

    return FWB_OK;
}

fwb_status_t fwb_crc_update(fwb_crc_ctx_t *ctx, const uint8_t *data, size_t len)
{
    if (ctx == NULL || (data == NULL && len != 0U)) {
        return FWB_ERR_INVALID_ARG;
    }

    /* Précondition : ctx doit avoir été initialisé par fwb_crc_init(). */
    if (!fwb_crc_width_valid(ctx->params.width)) {
        return FWB_ERR_INVALID_ARG;
    }

    /* Traitement CRC à déléguer à la fonction interne. */
    ctx->reg = fwb_crc_state_update_bitwise(ctx->reg, data, len, &ctx->params);

    return FWB_OK;
}

fwb_status_t fwb_crc_finalize(const fwb_crc_ctx_t *ctx, uint32_t *result)
{
    if (ctx == NULL || result == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    /* Précondition : ctx doit avoir été initialisé par fwb_crc_init(). */
    if (!fwb_crc_width_valid(ctx->params.width)) {
        return FWB_ERR_INVALID_ARG;
    }

    *result = fwb_crc_state_finalize(ctx->reg, &ctx->params);

    return FWB_OK;
}

fwb_status_t fwb_crc_oneshot(const fwb_crc_params_t *params, const uint8_t *data, size_t len,
                             uint32_t *result)
{
    /* 1. Valider les arguments */
    if (params == NULL || result == NULL || (data == NULL && len != 0U)) {
        return FWB_ERR_INVALID_ARG;
    }

    /* 2. Réutiliser l'API publique */
    fwb_crc_ctx_t ctx;

    fwb_status_t status = fwb_crc_init(&ctx, params);
    if (status != FWB_OK) {
        return status;
    }

    status = fwb_crc_update(&ctx, data, len);
    if (status != FWB_OK) {
        return status;
    }

    return fwb_crc_finalize(&ctx, result);
}