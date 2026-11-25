#include "fwb/framing/framing.h"
#include "framing_internal.h"

#include "fwb/common/endian.h"
#include "fwb/crc/crc.h"

#include <string.h>

/* FRM-REQ-010 to 021 */

size_t fwb_framing_encoded_size(uint16_t payload_len)
{
    if ((size_t)payload_len > FWB_FRAMING_MAX_PAYLOAD) {
        return 0U;
    }
    size_t frame_len = FWB_FRAMING_HEADER_SIZE + (size_t)payload_len + FWB_FRAMING_CRC_SIZE;
    return frame_len + 1U + (frame_len / 254U) + 1U; /* COBS overhead + delimiter */
}

fwb_status_t fwb_framing_encode(const fwb_framing_cfg_t *cfg,
                                uint8_t type, uint16_t seq,
                                const uint8_t *payload, uint16_t payload_len,
                                uint8_t *dst, size_t dst_cap,
                                size_t *out_len)
{
    if (cfg == NULL || dst == NULL || out_len == NULL) {
        return FWB_ERR_INVALID_ARG;
    }
    if (payload == NULL && payload_len != 0U) {
        return FWB_ERR_INVALID_ARG; /* FRM-REQ-017 */
    }
    if ((size_t)payload_len > FWB_FRAMING_MAX_PAYLOAD) {
        return FWB_ERR_PAYLOAD_TOO_LARGE; /* FRM-REQ-015 */
    }

    size_t wire_size = fwb_framing_encoded_size(payload_len);
    if (dst_cap < wire_size) {
        return FWB_ERR_BUFFER_TOO_SMALL; /* FRM-REQ-016: no bytes written */
    }

    /* Build decoded frame in a local buffer. */
    uint8_t frame[FWB_FRAMING_MAX_FRAME_SIZE];
    size_t  frame_len = FWB_FRAMING_HEADER_SIZE + (size_t)payload_len + FWB_FRAMING_CRC_SIZE;

    /* FRM-REQ-011, 012, 013: header */
    frame[0] = (uint8_t)FWB_FRAMING_VERSION; /* VER  */
    frame[1] = 0x00U;                        /* FLAGS */
    frame[2] = type;                         /* TYPE  */
    fwb_wr_u16_le(&frame[3], seq);           /* SEQ LE */
    fwb_wr_u16_le(&frame[5], payload_len);   /* LEN LE */

    /* FRM-REQ-018: payload is read-only; copied into frame buffer. */
    if (payload_len > 0U) {
        memcpy(&frame[7], payload, (size_t)payload_len);
    }

    /* FRM-REQ-014: CRC over link_id (LE) || header || payload. */
    uint8_t  link_id_bytes[2];
    fwb_wr_u16_le(link_id_bytes, cfg->link_id);

    fwb_crc_ctx_t crc_ctx;
    (void)fwb_crc_init(&crc_ctx, &fwb_crc_params_ccitt_false);
    (void)fwb_crc_update(&crc_ctx, link_id_bytes, 2U);
    (void)fwb_crc_update(&crc_ctx, frame, FWB_FRAMING_HEADER_SIZE + (size_t)payload_len);
    uint32_t crc_val = 0U;
    (void)fwb_crc_finalize(&crc_ctx, &crc_val);
    fwb_wr_u16_le(&frame[FWB_FRAMING_HEADER_SIZE + (size_t)payload_len], (uint16_t)crc_val);

    /* COBS encode into dst, then append delimiter. */
    size_t cobs_len = framing_cobs_encode(frame, frame_len, dst, dst_cap - 1U);
    dst[cobs_len] = 0x00U;
    *out_len = cobs_len + 1U; /* FRM-REQ-020 */

    return FWB_OK;
}
