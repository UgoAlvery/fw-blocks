#include <string.h>

#include "fwb/framing/framing.h"

#include "framing_internal.h"

/* FRM-REQ-080 to 090 */

fwb_status_t fwb_framing_rx_init(fwb_framing_rx_t *rx, const fwb_framing_cfg_t *cfg,
                                 uint16_t timeout_ticks)
{
    if (rx == NULL || cfg == NULL) {
        return FWB_ERR_INVALID_ARG;
    }
    rx->cfg = *cfg;
    rx->state = FWB_FRAMING_RX_IDLE;
    rx->buf_len = 0U;
    rx->timeout_ticks = timeout_ticks;
    rx->idle_ticks = 0U;
    return FWB_OK;
}

/* FRM-REQ-089: reset to IDLE, preserve cfg and timeout_ticks. */
void fwb_framing_rx_reset(fwb_framing_rx_t *rx)
{
    if (rx == NULL) {
        return;
    }
    rx->state = FWB_FRAMING_RX_IDLE;
    rx->buf_len = 0U;
    rx->idle_ticks = 0U;
}

fwb_status_t fwb_framing_rx_push(fwb_framing_rx_t *rx, uint8_t byte, fwb_framing_frame_t *out,
                                 bool *frame_ready)
{
    if (rx == NULL || out == NULL || frame_ready == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    *frame_ready = false;
    rx->idle_ticks = 0U;

    switch (rx->state) {
    case FWB_FRAMING_RX_IDLE:
        if (byte == 0x00U) {
            /* Empty frame: ignore silently (FRM-REQ-082). */
            return FWB_OK;
        }
        rx->buf_len = 0U;
        rx->buf[rx->buf_len++] = byte;
        rx->state = FWB_FRAMING_RX_RECEIVING;
        return FWB_OK;

    case FWB_FRAMING_RX_RECEIVING:
        if (byte != 0x00U) {
            if (rx->buf_len >= sizeof(rx->buf)) {
                /* Buffer full: overflow (FRM-REQ-083). */
                rx->state = FWB_FRAMING_RX_DISCARDING;
                rx->buf_len = 0U;
                return FWB_ERR_OVERFLOW;
            }
            rx->buf[rx->buf_len++] = byte;
            return FWB_OK;
        }
        /* Delimiter received: decode and validate (rx-state-machine.md §4). */
        {
            rx->state = FWB_FRAMING_RX_IDLE;

            if (rx->buf_len == 0U) {
                /* Empty COBS payload after delimiter: ignore. */
                return FWB_OK;
            }

            uint8_t frame_buf[FWB_FRAMING_MAX_FRAME_SIZE];
            size_t frame_len = 0U;
            fwb_status_t st =
                framing_cobs_decode(rx->buf, rx->buf_len, frame_buf, sizeof(frame_buf), &frame_len);
            rx->buf_len = 0U;

            if (st != FWB_OK) {
                return st; /* FWB_ERR_COBS_MALFORMED */
            }

            st = fwb_framing_decode(&rx->cfg, frame_buf, frame_len, out);
            if (st != FWB_OK) {
                return st;
            }

            *frame_ready = true;
            return FWB_OK;
        }

    case FWB_FRAMING_RX_DISCARDING:
        if (byte == 0x00U) {
            /* Delimiter seen: resync (FRM-REQ-084). */
            rx->state = FWB_FRAMING_RX_IDLE;
            rx->buf_len = 0U;
        }
        return FWB_OK;

    default:
        return FWB_ERR_INVALID_ARG;
    }
}

fwb_status_t fwb_framing_rx_push_buf(fwb_framing_rx_t *rx, const uint8_t *data, size_t len,
                                     size_t *consumed, fwb_framing_frame_t *out, bool *frame_ready)
{
    if (rx == NULL || (data == NULL && len != 0U) || consumed == NULL || out == NULL ||
        frame_ready == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    *frame_ready = false;
    *consumed = 0U;

    for (size_t i = 0U; i < len; i++) {
        fwb_status_t st = fwb_framing_rx_push(rx, data[i], out, frame_ready);
        (*consumed)++;
        if (*frame_ready || st != FWB_OK) {
            return st; /* FRM-REQ-081: same result as byte-by-byte */
        }
    }
    return FWB_OK;
}

/* FRM-REQ-086, 087, 088 */
fwb_status_t fwb_framing_rx_tick(fwb_framing_rx_t *rx)
{
    if (rx == NULL) {
        return FWB_ERR_INVALID_ARG;
    }

    if (rx->state != FWB_FRAMING_RX_RECEIVING) {
        /* FRM-REQ-088: timeout in IDLE or DISCARDING produces no error. */
        return FWB_OK;
    }

    if (rx->timeout_ticks == 0U) {
        return FWB_OK; /* Timeout disabled. */
    }

    rx->idle_ticks++;
    if (rx->idle_ticks >= rx->timeout_ticks) {
        rx->state = FWB_FRAMING_RX_IDLE;
        rx->buf_len = 0U;
        rx->idle_ticks = 0U;
        return FWB_ERR_TIMEOUT; /* FRM-REQ-087 */
    }
    return FWB_OK;
}
