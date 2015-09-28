/**
 * device_uplink_common.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef DEVICE_UPLINK_COMMON_H_
#define DEVICE_UPLINK_COMMON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "gw_beacon.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * bits 1: is_need_ack?
 * bits 2: _reserved
 * bits 2: retransmit_num   \sa LPWAN_DE_MAX_RETRANSMIT_NUM
 * bits 3: tx_fail_num      \sa LPWAN_DE_MAX_TX_FAIL_NUM
 */
typedef os_uint8 device_uplink_common_hdr_t;

struct device_uplink_common {
    device_uplink_common_hdr_t hdr;
    beacon_seq_t beacon_seq;    /**< introduce some variance, for security concerns */
    os_int8 beacon_rssi;
    os_int8 beacon_snr;
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_UPLINK_COMMON_H_ */
