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

struct parsed_device_uplink_msg_info {
    os_int8 beacon_seq_id;
    os_uint8 beacon_class_seq_id;

    os_int8 beacon_rssi;
    os_int8 beacon_snr;

    os_uint8 seq;

    enum device_message_type type;
    os_uint8 msg_len;

    os_boolean is_need_ack;

    os_uint8 retransmit_num;
    os_uint8 tx_fail_num;
};

__LPWAN os_int8 lpwan_parse_device_uplink_msg(struct device_uplink_msg *up_msg,
                                              os_uint8 len,
                                              struct parsed_device_uplink_msg_info *info);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_DEVICE_UPLINK_MSG_H_ */
