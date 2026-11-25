#include "fwb/antireplay/antireplay.h"

#include <string.h>

_Static_assert(sizeof(fwb_antireplay_ctx_t) <= 16U, "AR-REQ-006: state > 16 bytes");

/* Mask covering exactly FWB_ANTIREPLAY_WINDOW bits.
 * #if avoids (1ULL << 64) which is UB and confuses static analyzers. */
#if FWB_ANTIREPLAY_WINDOW == 64U
#define AR_WINDOW_MASK UINT64_MAX
#else
#define AR_WINDOW_MASK (((uint64_t)1U << FWB_ANTIREPLAY_WINDOW) - 1U)
#endif

fwb_status_t fwb_antireplay_init(fwb_antireplay_ctx_t *ctx)
{
    if (ctx == NULL) {
        return FWB_ERR_INVALID_ARG;
    }
    memset(ctx, 0, sizeof(*ctx));
    return FWB_OK;
}

bool fwb_antireplay_is_seeded(const fwb_antireplay_ctx_t *ctx)
{
    if (ctx == NULL) {
        return false;
    }
    return ctx->seeded;
}

fwb_status_t fwb_antireplay_reset(fwb_antireplay_ctx_t *ctx)
{
    return fwb_antireplay_init(ctx);
}

fwb_status_t fwb_antireplay_check(fwb_antireplay_ctx_t *ctx, uint16_t seq)
{
    if (ctx == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    /* AR-REQ-011, 012: first value accepted unconditionally. */
    if (!ctx->seeded) {
        ctx->last_seq = seq;
        ctx->mask     = 1U;
        ctx->seeded   = true;
        return FWB_OK;
    }

    /* AR-REQ-032: unsigned subtraction then reinterpret as signed — no UB.
     * Handles 0xFFFF→0x0000 wrap correctly (AR-REQ-030). */
    int16_t delta = (int16_t)((uint16_t)(seq - ctx->last_seq));

    if (delta == 0) {
        return FWB_ERR_REPLAY; /* AR-REQ-021 */
    }

    if (delta > 0) {
        fwb_status_t result = (delta == 1) ? FWB_OK : FWB_ERR_SEQ_GAP; /* AR-REQ-025, 026 */

        /* AR-REQ-028: delta >= W → clear mask, no shift (would be UB for W=64). */
        if ((uint16_t)delta >= (uint16_t)FWB_ANTIREPLAY_WINDOW) {
            ctx->mask = 1U;
        } else {
            ctx->mask = ((ctx->mask << ((uint8_t)delta & 63U)) & AR_WINDOW_MASK) | 1U;
        }
        ctx->last_seq = seq;
        return result;
    }

    /* delta < 0: backward lookup. */
    if (delta <= -(int32_t)FWB_ANTIREPLAY_WINDOW) {
        return FWB_ERR_TOO_OLD; /* AR-REQ-024 */
    }

    /* -W < delta < 0: check and mark. AR-REQ-022, 023, 027. */
    uint8_t bit_pos = (uint8_t)(-(int32_t)delta); /* 1..W-1 */
    if (((ctx->mask >> bit_pos) & 1U) != 0U) {
        return FWB_ERR_REPLAY;
    }
    ctx->mask |= ((uint64_t)1U << bit_pos);
    return FWB_OK;
}
