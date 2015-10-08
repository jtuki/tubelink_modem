/**
 * parse_device_uplink_common.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PARSE_DEVICE_UPLINK_COMMON_H_
#define PARSE_DEVICE_UPLINK_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "frame_defs/device_uplink_common.h"

struct parsed_device_uplink_common {
    os_int8 beacon_seq_id;
    os_uint8 beacon_class_seq_id;

    os_int8 beacon_rssi;
    os_int8 beacon_snr;

    os_boolean is_need_ack;

    os_uint8 retransmit_num;
    os_uint8 tx_fail_num;
};

__LPWAN void lpwan_parse_device_uplink_common(const struct device_uplink_common *up_common,
                                              os_uint8 len,
                                              struct parsed_device_uplink_common *common);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_DEVICE_UPLINK_COMMON_H_ */
