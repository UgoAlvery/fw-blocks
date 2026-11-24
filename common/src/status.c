#include "fwb/common/status.h"

#include <stdint.h>

_Static_assert(sizeof(uint8_t) == 1, "fw-blocks : uint8_t doit valoir 1 octet");
_Static_assert(sizeof(uint16_t) == 2, "fw-blocks : uint16_t doit valoir 2 octets");
_Static_assert(sizeof(uint32_t) == 4, "fw-blocks : uint32_t doit valoir 4 octets");

#ifndef FWB_NO_STATUS_STRINGS

const char *fwb_status_str(fwb_status_t s)
{
    switch (s) {
    case FWB_OK:
        return "FWB_OK";
    case FWB_ERR_INVALID_ARG:
        return "FWB_ERR_INVALID_ARG";
    case FWB_ERR_BUFFER_TOO_SMALL:
        return "FWB_ERR_BUFFER_TOO_SMALL";
    case FWB_ERR_NOT_SUPPORTED:
        return "FWB_ERR_NOT_SUPPORTED";
    case FWB_ERR_FRAME_TOO_SHORT:
        return "FWB_ERR_FRAME_TOO_SHORT";
    case FWB_ERR_PAYLOAD_TOO_LARGE:
        return "FWB_ERR_PAYLOAD_TOO_LARGE";
    case FWB_ERR_LENGTH_MISMATCH:
        return "FWB_ERR_LENGTH_MISMATCH";
    case FWB_ERR_BAD_CRC:
        return "FWB_ERR_BAD_CRC";
    case FWB_ERR_BAD_VERSION:
        return "FWB_ERR_BAD_VERSION";
    case FWB_ERR_BAD_FLAGS:
        return "FWB_ERR_BAD_FLAGS";
    case FWB_ERR_COBS_MALFORMED:
        return "FWB_ERR_COBS_MALFORMED";
    case FWB_ERR_OVERFLOW:
        return "FWB_ERR_OVERFLOW";
    case FWB_ERR_TIMEOUT:
        return "FWB_ERR_TIMEOUT";
    case FWB_ERR_REPLAY:
        return "FWB_ERR_REPLAY";
    case FWB_ERR_TOO_OLD:
        return "FWB_ERR_TOO_OLD";
    case FWB_ERR_SEQ_GAP:
        return "FWB_ERR_SEQ_GAP";
    default:
        return "(inconnu)";
    }
}

#endif /* FWB_NO_STATUS_STRINGS */
