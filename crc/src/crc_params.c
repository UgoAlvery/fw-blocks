#include "fwb/crc/crc.h"

const fwb_crc_params_t fwb_crc_params_ccitt_false = {16U, 0x1021U, 0xFFFFU, 0x0000U, false, false};

const fwb_crc_params_t fwb_crc_params_crc32c = {32U,         0x1EDC6F41U, 0xFFFFFFFFU,
                                                0xFFFFFFFFU, true,        true};