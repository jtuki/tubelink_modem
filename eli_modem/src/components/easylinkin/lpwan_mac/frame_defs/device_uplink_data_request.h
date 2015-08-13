/**
 * device_uplink_data_request.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef DEVICE_UPLINK_DATA_REQUEST_H_
#define DEVICE_UPLINK_DATA_REQUEST_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "device_uplink_common.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * \remark Only join request, cmd, and message will increment the @seq, thus
 *      there is no need to put @seq here.
 * \sa struct device_uplink_ack
 */
__LPWAN struct device_uplink_data_requst {
    struct device_uplink_common hdr;
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_UPLINK_DATA_REQUEST_H_ */
