/**
 * parse_gw_join_response.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PARSE_GW_JOIN_RESPONSE_H_
#define PARSE_GW_JOIN_RESPONSE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "parsed_common_defs.h"

/**
 * \remark As to FTYPE_GW_JOIN_PENDING_ACK, refer to struct gw_downlink_ack.
 * 
 * \sa FTYPE_GW_JOIN_CONFIRMED
 * \sa struct gw_join_confirmed
 */
struct parsed_gw_join_confirmed {
    os_uint8 init_seq_id;
    enum gw_join_confirmed_info info;
    short_addr_t distributed_short_addr;
};

/**
 * \return -1 if parse failed, len otherwise.
 */
__LPWAN
os_int8 lpwan_parse_gw_join_confirmed (const void *join_confirmed,
                                       os_uint8 len,
                                       struct parsed_gw_join_confirmed *confirmed);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_GW_JOIN_RESPONSE_H_ */
