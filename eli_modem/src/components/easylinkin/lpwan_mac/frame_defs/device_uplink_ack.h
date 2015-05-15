/**
 * device_uplink_ack.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef DEVICE_UPLINK_ACK_H_
#define DEVICE_UPLINK_ACK_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "device_uplink_common.h"

#ifdef __GNUC__
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * \remark Only join request, cmd, and message will increment the @seq, thus
 *      there is no need to put @seq here.
 * \sa struct device_uplink_data_requst
 */
__LPWAN struct device_uplink_ack {
    struct device_uplink_common hdr;
    os_uint8 confirm_seq;
};

#ifdef __GNUC__
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_UPLINK_ACK_H_ */
