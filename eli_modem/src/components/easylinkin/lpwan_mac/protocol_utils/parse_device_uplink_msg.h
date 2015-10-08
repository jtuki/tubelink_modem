/**
 * parse_device_uplink_msg.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PARSE_DEVICE_UPLINK_MSG_H_
#define PARSE_DEVICE_UPLINK_MSG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "frame_defs/device_uplink_msg.h"
#include "parse_device_uplink_common.h"

/**
 * Parsed device's uplink message, including the uplink common.
 * \sa struct parsed_device_uplink_common
 */
struct parsed_device_uplink_msg_info {
    struct parsed_device_uplink_common up_common;

    os_uint8 seq;
    enum device_message_type type;
    os_uint8 msg_len;
};

__LPWAN os_int8 lpwan_parse_device_uplink_msg(const struct device_uplink_msg *up_msg,
                                              os_uint8 len,
                                              struct parsed_device_uplink_msg_info *info);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_DEVICE_UPLINK_MSG_H_ */
