#include <string.h>

#include "fwb/common/endian.h"
#include "fwb/crc/crc.h"
#include "fwb/framing/framing.h"

#include "framing_internal.h"

/* FRM-REQ-030 to 042: order of checks follows wire-format.md §7 exactly. */

fwb_status_t fwb_framing_decode(const fwb_framing_cfg_t *cfg, const uint8_t *frame,
                                size_t frame_len, fwb_framing_frame_t *out)
{
    if (cfg == NULL || frame == NULL || out == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    /* Check 1: n >= 9 (FRM-REQ-031) */
    if (frame_len < FWB_FRAMING_MIN_FRAME_SIZE) {
        return FWB_ERR_FRAME_TOO_SHORT;
    }

    /* Check 2: LEN <= MAX_PAYLOAD (FRM-REQ-032) */
    uint16_t len = fwb_rd_u16_le(&frame[5]);
    if ((size_t)len > FWB_FRAMING_MAX_PAYLOAD) {
        return FWB_ERR_PAYLOAD_TOO_LARGE;
    }

    /* Check 3: frame_len == 7 + LEN + 2 (FRM-REQ-033) */
    if (frame_len != FWB_FRAMING_HEADER_SIZE + (size_t)len + FWB_FRAMING_CRC_SIZE) {
        return FWB_ERR_LENGTH_MISMATCH;
    }

    /* Check 4: CRC (FRM-REQ-034, 037) */
    uint8_t link_id_bytes[2];
    fwb_wr_u16_le(link_id_bytes, cfg->link_id);

    fwb_crc_ctx_t crc_ctx;
    (void)fwb_crc_init(&crc_ctx, &fwb_crc_params_ccitt_false);
    (void)fwb_crc_update(&crc_ctx, link_id_bytes, 2U);
    (void)fwb_crc_update(&crc_ctx, frame, FWB_FRAMING_HEADER_SIZE + (size_t)len);
    uint32_t computed = 0U;
    (void)fwb_crc_finalize(&crc_ctx, &computed);

    uint16_t received = fwb_rd_u16_le(&frame[FWB_FRAMING_HEADER_SIZE + (size_t)len]);
    if ((uint16_t)computed != received) {
        return FWB_ERR_BAD_CRC;
    }

    /* Check 5: VER == 0x01 (FRM-REQ-035) */
    if (frame[0] != (uint8_t)FWB_FRAMING_VERSION) {
        return FWB_ERR_BAD_VERSION;
    }

    /* Check 6: FLAGS == 0x00 (FRM-REQ-036) */
    if (frame[1] != 0x00U) {
        return FWB_ERR_BAD_FLAGS;
    }

    /* FRM-REQ-039: only write to out on success. */
    out->type = frame[2];
    out->seq = fwb_rd_u16_le(&frame[3]);
    out->payload_len = len;
    if (len > 0U) {
        memcpy(out->payload, &frame[7], (size_t)len);
    }

    return FWB_OK;
}

fwb_status_t fwb_framing_decode_wire(const fwb_framing_cfg_t *cfg, const uint8_t *wire,
                                     size_t wire_len, fwb_framing_frame_t *out)
{
    if (cfg == NULL || wire == NULL || out == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    uint8_t frame_buf[FWB_FRAMING_MAX_FRAME_SIZE];
    size_t frame_len = 0U;

    fwb_status_t st = framing_cobs_decode(wire, wire_len, frame_buf, sizeof(frame_buf), &frame_len);
    if (st != FWB_OK) {
        return st;
    }

    return fwb_framing_decode(cfg, frame_buf, frame_len, out);
}
