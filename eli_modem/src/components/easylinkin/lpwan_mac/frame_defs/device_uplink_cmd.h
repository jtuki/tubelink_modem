/**
 * device_uplink_cmd.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef DEVICE_UPLINK_CMD_H_
#define DEVICE_UPLINK_CMD_H_

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

__LPWAN struct device_uplink_cmd_common {
    struct device_uplink_common hdr;
    os_uint8 seq;
};

/*---------------------------------------------------------------------------*/
/**< device cmd: active leave @{ */

__LPWAN struct device_cmd_leave {
    /**
     * bits 4: cmd_id   \sa enum cmd_id_end_device
     * bits 4: leave_reason \sa enum _device_leave_reason
     */
    os_uint8 cmd_info;
};

enum _device_leave_reason {
    DEVICE_LEAVE_REASON_DEFAULT = 0,
    _device_leave_reason_invalid = 16,
};

/**< @} */

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_UPLINK_CMD_H_ */
