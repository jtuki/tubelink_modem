/**
 * construct_de_uplink_common.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef CONSTRUCT_DE_UPLINK_COMMON_H_
#define CONSTRUCT_DE_UPLINK_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "frame_defs/device_uplink_common.h"

void update_device_uplink_common(struct device_uplink_common *up_comm,
                                 os_uint8 retrans, os_uint8 tx_fail,
                                 os_int8 bcn_seq_id, os_uint8 bcn_class_seq_id,
                                 os_int16 bcn_rssi, os_int16 bcn_snr);

#ifdef __cplusplus
}
#endif

#endif /* CONSTRUCT_DE_UPLINK_COMMON_H_ */
