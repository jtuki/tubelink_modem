/**
 * parsed_common_defs.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PARSED_COMMON_DEFS_H_
#define PARSED_COMMON_DEFS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "frame_defs/frame_defs.h"

#define ADDR_TYPE_SHORT_ADDRESS         0
#define ADDR_TYPE_MULTICAST_ADDRESS     1
#define ADDR_TYPE_MODEM_UUID            2
#define ADDR_TYPE_SHORTENED_MODEM_UUID  3

struct lpwan_addr {
    os_uint8 type; /**< \sa ADDR_TYPE_SHORT_ADDRESS etc. */
    union {
        short_addr_t short_addr;
        multicast_addr_t multi_addr;
        modem_uuid_t uuid;
        short_modem_uuid_t suuid;
    } addr;
};

/**
 * get bits from a byte
 * i.e. bits: [7 6 5 4 3 2 1 0]; for 0x80, get_bits(0x80, 7, 7) == 1
 */
#define get_bits(byte, end, start) ((((os_uint8) byte) >> (start)) \
                                   & ((os_uint8)((((os_uint16)1) << (end-start+1)) - 1)))

/**
 * set value for a byte between @end and @start.
 * i.e.
 *   os_uint8 a = 0;
 *   set_bits(a, 7, 6, 3) == 0xC0;
 */
#define set_bits(byte, end, start, value) do { \
    os_uint8 _all_one = (((os_uint8)1)<<((end)-(start))) - 1; \
    os_uint8 _single_one = ((os_uint8)1)<<((end)-(start)); \
    (byte) &= ~(((_all_one) + (_single_one))<<(start)); \
    (byte) += ((os_uint8) (value))<<(start); \
} while (0)

#ifdef __cplusplus
}
#endif

#endif /* PARSED_COMMON_DEFS_H_ */
