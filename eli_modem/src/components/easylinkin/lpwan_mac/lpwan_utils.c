/**
 * lpwan_utils.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lpwan_utils.h"

os_uint32 construct_u32_2(os_uint16 higher, os_uint16 lower)
{
    return ((os_uint32) higher << 16) + lower;
}

os_uint32 construct_u32_4(os_uint8 highest, os_uint8 high,
                              os_uint8 low, os_uint8 lowest)
{
    return ((os_uint32) highest << 24) + ((os_uint32) high << 16) + \
           ((os_uint32) low << 8) + lowest;
}

short_addr_t short_modem_uuid(modem_uuid_t uuid)
{
    os_uint16 higher = (os_uint16) uuid.addr[1] << 8;
    os_uint16 lower = uuid.addr[0];
    return (short_addr_t) (higher + lower);
}