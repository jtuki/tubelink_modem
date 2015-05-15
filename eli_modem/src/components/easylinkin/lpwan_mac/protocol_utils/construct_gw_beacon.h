/**
 * construct_gw_beacon.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef CONSTRUCT_GW_BEACON_H_
#define CONSTRUCT_GW_BEACON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "protocol_utils/parse_beacon.h"
#include "lpwan_types.h"

os_int8 construct_gateway_beacon_header(void *bcn_buffer,
                                        os_uint8 buffer_len,
                                        struct parsed_beacon_info *bcn_info);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRUCT_GW_BEACON_H_ */
