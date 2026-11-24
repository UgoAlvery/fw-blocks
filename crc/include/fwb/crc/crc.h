#ifndef FWB_CRC_CRC_H
#define FWB_CRC_CRC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "fwb/common/status.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Rocksoft model parameters. poly is in normal form, top bit implicit. */
typedef struct fwcrc_paramsb_ {
    uint8_t width;
    uint32_t poly;
    uint32_t init;
    uint32_t xorout;
    bool refin;
    bool refout;
} fwb_crc_params_t;

/* Caller-allocated context: no dynamic allocation (CRC-REQ-001).
 * Fields are private; do not touch them directly. */
typedef struct fwb_crc_ctx {
    uint32_t reg;
    fwb_crc_params_t params;
} fwb_crc_ctx_t;

/* Lookup table for the table-driven implementation (CRC-REQ-040/041).
 * Caller-allocated (1 KiB + params), built once per parameter set with
 * fwb_crc_table_build(). It keeps a copy of the params it was built for so
 * fwb_crc_update_table() can reject a mismatching ctx. */
typedef struct fwb_crc_table {
    uint32_t entries[256];
    fwb_crc_params_t params;
} fwb_crc_table_t;

/* Predefined variants (CRC-REQ-014, CRC-REQ-015). */
extern const fwb_crc_params_t fwb_crc_params_ccitt_false;
extern const fwb_crc_params_t fwb_crc_params_crc32c;

fwb_status_t fwb_crc_init(fwb_crc_ctx_t *ctx, const fwb_crc_params_t *params);

/* ctx must have been set up by a successful fwb_crc_init(); calling this
 * on an uninitialized context is undefined behavior. */
fwb_status_t fwb_crc_update(fwb_crc_ctx_t *ctx, const uint8_t *data, size_t len);

/* Table-driven variant of fwb_crc_update(). The table must have been built
 * with the same width/poly/refin as ctx, otherwise FWB_ERR_INVALID_ARG.
 * Same precondition as fwb_crc_update(): ctx initialized by fwb_crc_init(). */
fwb_status_t fwb_crc_table_build(const fwb_crc_params_t *params, fwb_crc_table_t *table);
fwb_status_t fwb_crc_update_table(fwb_crc_ctx_t *ctx, const fwb_crc_table_t *table,
                                  const uint8_t *data, size_t len);

fwb_status_t fwb_crc_finalize(const fwb_crc_ctx_t *ctx, uint32_t *result);
fwb_status_t fwb_crc_oneshot(const fwb_crc_params_t *params, const uint8_t *data, size_t len,
                             uint32_t *result);

#ifdef __cplusplus
}
#endif

#endif /* FWB_CRC_CRC_H */
