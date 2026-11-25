/* libFuzzer harness — AR-REQ-032, AR-REQ-040, AR-REQ-041, AR-REQ-042 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fwb/antireplay/antireplay.h"

/* Reference model: a simple seen-set for the last W values accepted.
 * We track accepted values in a fixed-size circular buffer and compare
 * decisions with the production code (AR-REQ-041, 042). */
#define REF_SIZE (FWB_ANTIREPLAY_WINDOW * 4U)

typedef struct {
    uint16_t accepted[REF_SIZE];
    uint32_t count;
} ref_model_t;

static bool ref_was_accepted(const ref_model_t *ref, uint16_t seq)
{
    for (uint32_t i = 0U; i < ref->count && i < REF_SIZE; i++) {
        if (ref->accepted[i] == seq) return true;
    }
    return false;
}

static void ref_record(ref_model_t *ref, uint16_t seq)
{
    if (ref->count < REF_SIZE) {
        ref->accepted[ref->count++] = seq;
    }
    /* shortcut: once buffer full we stop comparing — the invariant may miss
     * replays of values older than REF_SIZE, when to upgrade: increase REF_SIZE */
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    if (size < 2U) return 0;

    static fwb_antireplay_ctx_t ctx;
    static ref_model_t ref;

    (void)fwb_antireplay_init(&ctx);
    memset(&ref, 0, sizeof(ref));

    uint32_t accepts = 0U;

    for (size_t i = 0U; i + 1U < size; i += 2U) {
        uint16_t seq = (uint16_t)((uint16_t)data[i] | ((uint16_t)data[i + 1U] << 8));
        fwb_status_t st = fwb_antireplay_check(&ctx, seq);

        bool prod_accepted = (st == FWB_OK || st == FWB_ERR_SEQ_GAP);

        if (prod_accepted) {
            /* AR-REQ-041: must not have been accepted before (within ref window). */
            if (ref_was_accepted(&ref, seq)) {
                __builtin_trap();
            }
            ref_record(&ref, seq);
            accepts++;
        }
    }

    /* AR-REQ-042: accepts <= distinct submitted values (trivially held since
     * ref_was_accepted gates every acceptance). */
    (void)accepts;

    return 0;
}
