#ifndef FWB_ANTIREPLAY_ANTIREPLAY_H
#define FWB_ANTIREPLAY_ANTIREPLAY_H

#include <stdbool.h>
#include <stdint.h>

#include "fwb/common/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* AR-REQ-005: configurable to 8, 16, 32 or 64, default 64. */
#ifndef FWB_ANTIREPLAY_WINDOW
#    define FWB_ANTIREPLAY_WINDOW 64U
#endif

_Static_assert(FWB_ANTIREPLAY_WINDOW == 8U || FWB_ANTIREPLAY_WINDOW == 16U ||
                   FWB_ANTIREPLAY_WINDOW == 32U || FWB_ANTIREPLAY_WINDOW == 64U,
               "FWB_ANTIREPLAY_WINDOW must be 8, 16, 32 or 64");

/* AR-REQ-006: sizeof <= 16 for W=64 (verified by _Static_assert in .c). */
typedef struct {
    uint64_t mask; /* bit i set → (last_seq - i) was accepted */
    uint16_t last_seq;
    bool seeded;
} fwb_antireplay_ctx_t;

/* AR-REQ-004: all public functions return FWB_ERR_INVALID_ARG on null ctx. */

fwb_status_t fwb_antireplay_init(fwb_antireplay_ctx_t *ctx);

/* AR-REQ-013: query seeding state. */
bool fwb_antireplay_is_seeded(const fwb_antireplay_ctx_t *ctx);

/* AR-REQ-014: reset to unseeded state, same as a fresh init. */
fwb_status_t fwb_antireplay_reset(fwb_antireplay_ctx_t *ctx);

/* Submit a sequence number.
 * Returns:
 *   FWB_OK          — accepted, delta == 1 (AR-REQ-026)
 *   FWB_ERR_SEQ_GAP — accepted, delta > 1 (AR-REQ-025)
 *   FWB_ERR_REPLAY  — rejected, already seen or delta == 0 (AR-REQ-021/023)
 *   FWB_ERR_TOO_OLD — rejected, delta <= -W (AR-REQ-024)
 * On rejection ctx is not modified (AR-REQ-027). */
fwb_status_t fwb_antireplay_check(fwb_antireplay_ctx_t *ctx, uint16_t seq);

#ifdef __cplusplus
}
#endif

#endif /* FWB_ANTIREPLAY_ANTIREPLAY_H */
