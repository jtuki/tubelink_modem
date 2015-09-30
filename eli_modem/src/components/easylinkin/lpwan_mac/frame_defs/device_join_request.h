/**
 * device_join_request.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef DEVICE_JOIN_REQUEST_H_
#define DEVICE_JOIN_REQUEST_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "device_uplink_common.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * \remark Devices' join request will generate a random @init_seq_id as
 *  initial session sequence id.
 */
__LPWAN struct device_join_request {
    struct device_uplink_common hdr;
    os_uint8 init_seq_id;
    os_uint8 join_reason;  /**< \sa enum _device_join_reason */
    app_id_t app_id;
};

enum _device_join_reason {
    JOIN_REASON_DEFAULT                 = 0,
    JOIN_REASON_REBOOT                  = 1,    /**< restart of modem */
    JOIN_REASON_LEAVE                   = 2,    /**< modem actively leave, then rejoin */
    /**< \sa enum _gw_cmd_force_leave_reason @{ */
    JOIN_REASON_FORCE_LEAVE_LOST_SERVER = 3,    /**< GW_FORCE_LEAVE_REASON_SERVER_UNREACHABLE */
    JOIN_REASON_FORCE_LEAVE_JOIN_NEARBY = 4,    /**< GW_FORCE_LEAVE_REASON_JOIN_NEARBY_GW */
    JOIN_REASON_FORCE_LEAVE_OUT_OF_FEE  = 5,    /**< GW_FORCE_LEAVE_REASON_OUT_OF_FEE_LEAVE */
    JOIN_REASON_FORCE_LEAVE_NO_ADDR     = 6,    /**< GW_FORCE_LEAVE_INVALID_SHORT_ADDR */
    /**< @} */
    _device_join_reason_invalid = 16,
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_JOIN_REQUEST_H_ */
