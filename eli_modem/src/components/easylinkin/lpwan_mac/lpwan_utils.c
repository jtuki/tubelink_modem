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

short_modem_uuid_t short_modem_uuid(modem_uuid_t *uuid)
{
    os_uint8 x0 = uuid->addr[11];
    os_uint8 x1 = uuid->addr[10];
    os_uint8 y0 = uuid->addr[9];
    os_uint8 y1 = uuid->addr[8];
    os_uint8 wafer_number = uuid->addr[7];

    os_uint16 higher = (os_uint16) (((x0 + x1) ^ (wafer_number + 91)) << 8);
    os_uint16 lower  = (os_uint16) ((y0 + y1) ^ (wafer_number + 91));
    return (short_modem_uuid_t) (higher + lower);
}
