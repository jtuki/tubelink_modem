/**
 * parse_device_uplink_msg.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "parsed_common_defs.h"
#include "parse_device_uplink_msg.h"

os_int8 lpwan_parse_device_uplink_msg(struct device_uplink_msg *up_msg,
                                      os_uint8 len,
                                      struct parsed_device_uplink_msg_info *info)
{
    os_uint8 _msg_len = get_bits(up_msg->type_and_len, 5, 0);
    if (sizeof(struct device_uplink_msg) + _msg_len != len)
        return -1;

    info->beacon_seq_id = up_msg->hdr.beacon_seq[0];
    info->beacon_class_seq_id = get_bits(up_msg->hdr.beacon_seq[1], 3, 0);

    info->beacon_lqi = up_msg->hdr.beacon_lqi;
    info->beacon_rssi = up_msg->hdr.beacon_rssi;

    info->type = (enum device_message_type) get_bits(up_msg->type_and_len, 7, 6);
    info->msg_len = _msg_len;

    info->seq = up_msg->seq;

    /** parse the @device_uplink_common_hdr_t */
    info->is_need_ack = get_bits(up_msg->hdr.hdr, 7, 7);
    info->contain_multicast_ack = get_bits(up_msg->hdr.hdr, 6, 6);
    info->contain_debug_information = get_bits(up_msg->hdr.hdr, 5, 5);
    info->retransmit_num = get_bits(up_msg->hdr.hdr, 4, 3);
    info->channel_backoff_num = get_bits(up_msg->hdr.hdr, 2, 0);

    return 0;
}
