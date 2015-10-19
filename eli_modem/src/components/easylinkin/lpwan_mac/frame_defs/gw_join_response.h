/**
 * gw_join_response.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef GW_JOIN_RESPONSE_H_
#define GW_JOIN_RESPONSE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "common_defs.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * Gateway's join response.
 *
 * \remark Destination are modem_uuid_t (12 bytes long).
 * \sa struct frame_header
 */
__LPWAN struct gw_join_confirmed {
    /**
     * bits 4: join_confirmed_info (\sa enum gw_join_confirmed_info)
     * btis 4: _reserved
     */
    os_uint8 hdr;
    os_uint8 init_seq_id;  /**< \remark gateway's join confirmed response will generate a
                             random @init_seq_id as the initial session downlink sequence id. */
    short_addr_t distributed_short_addr;
};

/**
 * \sa LPWAN_UNPAID_MINIMUM_ROUTINE_INTERVAL
 * If not paid, the modem can only send routine messages with minimum
 * interval LPWAN_UNPAID_MINIMUM_ROUTINE_INTERVAL (seconds).
 */
enum gw_join_confirmed_info {
    JOIN_CONFIRMED_PAID         = 0,
    JOIN_CONFIRMED_NOT_PAID     = 1,
    JOIN_DENIED_DEFAULT         = 2, /**< no concrete reason */
    JOIN_DENIED_LOCATION_ERR    = 3, /**< some device may be restricted
                                        to connect to gateways within
                                        specific area */
    _join_confirmed_info_invalid = 4,
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* GW_JOIN_RESPONSE_H_ */
