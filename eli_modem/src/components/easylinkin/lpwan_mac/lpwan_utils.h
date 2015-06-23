/**
 * lpwan_utils.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef LPWAN_UTILS_H_
#define LPWAN_UTILS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "frame_defs/common_defs.h"

/**
 * \brief Set the bit-vector.
 */
#define BV(n) (((os_uint32)1)<<(n))

/**
 * Assertions.
 */
#ifdef LPWAN_DEBUG
#define lpwan_assert(x) do {if (!(x)) while(1) {;}} while (0)
#else
#define lpwan_assert(x)
#endif

/**
 * Construction of uint32 contents. From 2 uint16 or 4 uint8.
 */
os_uint32 construct_u32_2(os_uint16 higher, os_uint16 lower);
os_uint32 construct_u32_4(os_uint8 highest, os_uint8 high,
                              os_uint8 low, os_uint8 lowest);

short_modem_uuid_t short_modem_uuid(modem_uuid_t *uuid);

#ifdef __cplusplus
}
#endif

#endif /* LPWAN_UTILS_H_ */
