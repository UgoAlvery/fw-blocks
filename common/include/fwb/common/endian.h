#ifndef FWB_COMMON_ENDIAN_H
#define FWB_COMMON_ENDIAN_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline uint16_t fwb_rd_u16_le(const uint8_t *p)
{
    return (uint16_t)((uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8));
}

static inline void fwb_wr_u16_le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8U);
}

static inline uint32_t fwb_rd_u32_le(const uint8_t *p)
{
    return ((uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) |
            ((uint32_t)p[3] << 24U));
}

static inline void fwb_wr_u32_le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8U);
    p[2] = (uint8_t)(v >> 16U);
    p[3] = (uint8_t)(v >> 24U);
}

static inline uint16_t fwb_rd_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static inline void fwb_wr_u16_be(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v >> 8U);
    p[1] = (uint8_t)v;
}

static inline uint32_t fwb_rd_u32_be(const uint8_t *p)
{
    return (((uint32_t)p[0] << 24U) | ((uint32_t)p[1] << 16U) | ((uint32_t)p[2] << 8U) |
            (uint32_t)p[3]);
}

static inline void fwb_wr_u32_be(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24U);
    p[1] = (uint8_t)(v >> 16U);
    p[2] = (uint8_t)(v >> 8U);
    p[3] = (uint8_t)v;
}

#ifdef __cplusplus
}
#endif

#endif /* FWB_COMMON_ENDIAN_H */
