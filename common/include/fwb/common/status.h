#ifndef FWB_COMMON_STATUS_H
#define FWB_COMMON_STATUS_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

_Static_assert(CHAR_BIT == 8, "fw-blocks : CHAR_BIT == 8 requis");

typedef enum {
    FWB_OK = 0,
    FWB_ERR_INVALID_ARG = -1,
    FWB_ERR_BUFFER_TOO_SMALL = -2,
    FWB_ERR_NOT_SUPPORTED = -3,
    FWB_ERR_FRAME_TOO_SHORT = -10,
    FWB_ERR_PAYLOAD_TOO_LARGE = -11,
    FWB_ERR_LENGTH_MISMATCH = -12,
    FWB_ERR_BAD_CRC = -13,
    FWB_ERR_BAD_VERSION = -14,
    FWB_ERR_BAD_FLAGS = -15,
    FWB_ERR_COBS_MALFORMED = -16,
    FWB_ERR_OVERFLOW = -17,
    FWB_ERR_TIMEOUT = -18,
    FWB_ERR_REPLAY = -20,
    FWB_ERR_TOO_OLD = -21,
    /* FWB_ERR_SEQ_GAP est une information : la trame est acceptee. */
    FWB_ERR_SEQ_GAP = -22,
} fwb_status_t;

#ifndef FWB_NO_STATUS_STRINGS
const char *fwb_status_str(fwb_status_t s);
#endif

#ifdef __cplusplus
}
#endif

#endif /* FWB_COMMON_STATUS_H */
