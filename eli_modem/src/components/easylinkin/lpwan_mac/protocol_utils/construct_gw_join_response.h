/**
 * construct_gw_join_response.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef CONSTRUCT_GW_JOIN_RESPONSE_H_
#define CONSTRUCT_GW_JOIN_RESPONSE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "frame_defs/frame_defs.h"
#include "parsed_common_defs.h"

os_int8 construct_gateway_join_response(void *frame_buffer, os_uint8 buffer_len,
                                        const modem_uuid_t *uuid,
                                        short_addr_t gw_cluster_addr,
                                        const struct gw_join_confirmed *join_response);

os_int8 construct_gateway_downlink_msg(void *frame_buffer, os_uint8 buffer_len,
                                       short_addr_t sn_short, short_addr_t gw_cluster_addr,
                                       const os_uint8 msg[], os_uint8 msg_len);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRUCT_GW_JOIN_RESPONSE_H_ */
