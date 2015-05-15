/**
 * device_uplink_msg.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef DEVICE_UPLINK_MSG_H_
#define DEVICE_UPLINK_MSG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "device_uplink_common.h"

#ifdef __GNUC__
#pragma pack(push)
#pragma pack(1)
#endif

__LPWAN struct device_uplink_msg {
    struct device_uplink_common hdr;
    os_uint8 seq;
    /**
     * bits 2: type \sa enum device_message_type (emergent/event/routine)
     * bits 6: len  \sa LPWAN_MAX_PAYLAOD_LEN
     */
    os_uint8 type_and_len;
    os_uint8 msg[0];
};

#ifdef __GNUC__
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_UPLINK_MSG_H_ */
