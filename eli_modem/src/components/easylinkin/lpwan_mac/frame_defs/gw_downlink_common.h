/**
 * gw_downlink_common.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef GW_DOWNLINK_COMMON_H_
#define GW_DOWNLINK_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"

#ifdef __GNUC__
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * The common header part, for gateway's downlink ACK, cmd, and message.
 * bits 2: preferred_next_tx_power
 * bits 1: is_need_ack? (need immediately MAC-layer acknowledgement)
 * bits 1: is_data_pending?
 * bits 1: is_send_immediately?
 * bits 3: wait_after
 *  \remark Wait after @wait_after beacon periods.
 *          If set 0, the modem should wait until previously specified
 *          downlink OP2 period. Else, delay @pollAfter beacon period
 *          to receive message (OP2).
 *          No use if @is_send_immediately is set.
 */
typedef os_uint8 gw_downlink_common_hdr_t;

#ifdef __GNUC__
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* GW_DOWNLINK_COMMON_H_ */
