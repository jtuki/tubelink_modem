/**
 * parse_device_uplink_msg.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "parsed_common_defs.h"
#include "parse_device_uplink_common.h"
#include "parse_device_uplink_msg.h"


os_int8 lpwan_parse_device_uplink_msg(const struct device_uplink_msg *up_msg,
                                      os_uint8 len,
                                      struct parsed_device_uplink_msg_info *info)
{
    os_uint8 _msg_len = get_bits(up_msg->type_and_len, 5, 0);
    if (sizeof(struct device_uplink_msg) + _msg_len != len)
        return -1;

    lpwan_parse_device_uplink_common(& up_msg->hdr, len, & info->up_common);

    info->seq = up_msg->seq;
    info->type = (enum device_message_type) get_bits(up_msg->type_and_len, 7, 6);
    info->msg_len = _msg_len;

    return 0;
}
