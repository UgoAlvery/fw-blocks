#ifndef FWB_FRAMING_FRAMING_H
#define FWB_FRAMING_FRAMING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fwb/common/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* FRM-REQ-005: configurable at compile time, default 256. */
#ifndef FWB_FRAMING_MAX_PAYLOAD
#define FWB_FRAMING_MAX_PAYLOAD 256U
#endif

/* FRM-REQ-004: all usable in static_assert and array declarations. */
#define FWB_FRAMING_VERSION        1U
#define FWB_FRAMING_HEADER_SIZE    7U
#define FWB_FRAMING_CRC_SIZE       2U
#define FWB_FRAMING_MIN_FRAME_SIZE 9U
#define FWB_FRAMING_MAX_FRAME_SIZE \
    (FWB_FRAMING_HEADER_SIZE + FWB_FRAMING_MAX_PAYLOAD + FWB_FRAMING_CRC_SIZE)
/* COBS overhead: n + 1 + floor(n/254) bytes, plus 0x00 delimiter. */
#define FWB_FRAMING_MAX_WIRE_SIZE \
    (FWB_FRAMING_MAX_FRAME_SIZE + 1U + (FWB_FRAMING_MAX_FRAME_SIZE / 254U) + 1U)

typedef struct {
    uint16_t link_id;
} fwb_framing_cfg_t;

typedef struct {
    uint8_t  type;
    uint16_t seq;
    uint16_t payload_len;
    uint8_t  payload[FWB_FRAMING_MAX_PAYLOAD];
} fwb_framing_frame_t;

typedef enum {
    FWB_FRAMING_RX_IDLE,
    FWB_FRAMING_RX_RECEIVING,
    FWB_FRAMING_RX_DISCARDING,
} fwb_framing_rx_state_t;

/* Caller-allocated rx context. Treat as opaque; do not access fields directly. */
typedef struct {
    fwb_framing_cfg_t      cfg;
    fwb_framing_rx_state_t state;
    uint8_t                buf[FWB_FRAMING_MAX_WIRE_SIZE - 1U];
    size_t                 buf_len;
    uint16_t               timeout_ticks;
    uint16_t               idle_ticks;
} fwb_framing_rx_t;

/* Returns max wire size (delimiter included) for a given payload length.
 * Returns 0 if payload_len > FWB_FRAMING_MAX_PAYLOAD. */
size_t fwb_framing_encoded_size(uint16_t payload_len);

fwb_status_t fwb_framing_encode(const fwb_framing_cfg_t *cfg,
                                uint8_t type, uint16_t seq,
                                const uint8_t *payload, uint16_t payload_len,
                                uint8_t *dst, size_t dst_cap,
                                size_t *out_len);

/* Validates a decoded frame (after COBS) and fills out.
 * Does not modify frame; does not write to out on error (FRM-REQ-039, 042). */
fwb_status_t fwb_framing_decode(const fwb_framing_cfg_t *cfg,
                                const uint8_t *frame, size_t frame_len,
                                fwb_framing_frame_t *out);

/* COBS-decodes wire then calls fwb_framing_decode. Delimiter excluded. */
fwb_status_t fwb_framing_decode_wire(const fwb_framing_cfg_t *cfg,
                                     const uint8_t *wire, size_t wire_len,
                                     fwb_framing_frame_t *out);

fwb_status_t fwb_framing_rx_init(fwb_framing_rx_t *rx,
                                 const fwb_framing_cfg_t *cfg,
                                 uint16_t timeout_ticks);

/* Resets to IDLE, preserves configuration (FRM-REQ-089). */
void fwb_framing_rx_reset(fwb_framing_rx_t *rx);

fwb_status_t fwb_framing_rx_push(fwb_framing_rx_t *rx, uint8_t byte,
                                 fwb_framing_frame_t *out, bool *frame_ready);

fwb_status_t fwb_framing_rx_push_buf(fwb_framing_rx_t *rx,
                                     const uint8_t *data, size_t len,
                                     size_t *consumed,
                                     fwb_framing_frame_t *out,
                                     bool *frame_ready);

/* Signals one tick of inactivity. Returns FWB_ERR_TIMEOUT if the in-progress
 * frame is abandoned, FWB_OK otherwise (FRM-REQ-086, 087, 088). */
fwb_status_t fwb_framing_rx_tick(fwb_framing_rx_t *rx);

#ifdef __cplusplus
}
#endif

#endif /* FWB_FRAMING_FRAMING_H */
