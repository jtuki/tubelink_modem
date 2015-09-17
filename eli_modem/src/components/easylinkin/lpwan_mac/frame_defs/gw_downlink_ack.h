/**
 * gw_downlink_ack.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef GW_DOWNLINK_ACK_H_
#define GW_DOWNLINK_ACK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "gw_downlink_common.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * \remark Only join confirmed, cmd, and message will increment the @seq, thus
 *      there is no need to put @seq here.
 * \sa struct gw_downlink_cmd_common; struct gw_downlink_msg;
 */
__LPWAN struct gw_downlink_ack {    // no use currently
    gw_downlink_common_hdr_t hdr;
    os_uint8 confirm_seq;
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* GW_DOWNLINK_ACK_H_ */
