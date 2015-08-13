/**
 * gw_downlink_msg.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef GW_DOWNLINK_MSG_H_
#define GW_DOWNLINK_MSG_H_

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
 * \remark
 *      If the @is_multicast_dest is set in struct frame_header, the @seq
 *      is the multicast message's sequence number for the specific multicast
 *      group.
 */
__LPWAN struct gw_downlink_msg {
    gw_downlink_common_hdr_t hdr;
    os_uint8 seq;
    os_uint8 len;      /**< \sa LPWAN_MAX_PAYLAOD_LEN */
    os_uint8 msg[];
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* GW_DOWNLINK_MSG_H_ */
