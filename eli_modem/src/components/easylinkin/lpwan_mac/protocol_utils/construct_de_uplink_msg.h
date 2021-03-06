/**
 * construct_de_uplink_msg.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef CONSTRUCT_DE_UPLINK_MSG_H_
#define CONSTRUCT_DE_UPLINK_MSG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "frame_defs/common_defs.h"
#include "lpwan_types.h"

extern const char *device_msg_type_string[3];

void construct_device_uplink_msg(enum device_message_type type,
                                 const os_uint8 msg[], os_uint8 msg_len,
                                 void *buffer, os_uint8 buffer_len);

void update_device_uplink_msg(struct device_uplink_msg *msg,
                              os_uint8 frame_seq_id,
                              os_uint8 retrans, os_uint8 tx_fail,
                              os_int8 bcn_seq_id, os_uint8 bcn_class_seq_id,
                              os_int16 bcn_rssi, os_int16 bcn_snr);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRUCT_DE_UPLINK_MSG_H_ */
