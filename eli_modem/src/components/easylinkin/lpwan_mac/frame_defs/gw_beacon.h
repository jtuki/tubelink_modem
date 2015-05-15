/**
 * gw_beacon.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef GW_BEACON_H_
#define GW_BEACON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "common_defs.h"

#ifdef __GNUC__
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * Beacon structure:
 *      1. struct beacon_header hdr;
 *      2. (has pending msg/cmd in op2? \sa @has_op2; @beacon_period_section_ratio_t)
 *      [optional] beacon_op2_hdr_t hdr;
 *      [optional] uint8 op2_msg_num;
 *      [optional] struct beacon_op2_msg op2_msg[@op2_msg_num];
 *      3. (has packed ack to notify? \sa @has_packed_ack)
 *      [optional] uint8 packed_ack_num;
 *      [optional] struct beacon_packed_ack packed_ack[@packed_ack_num];
 * @{
 */

/**
 * \brief Packed bits-field which contains the beacon's information.
 * 
 * byte 1:
 *      bits 2: beacon_period_length (~ constant) \sa enum _beacon_period
 *      bits 2: beacon_groups_num (~ constant) \sa enum _beacon_max_groups_num
 *      bits 4: beacon_classes_num (var, but nearly constant, at most 16 classes)
 * byte 2:
 *      bits 1: has_op2? (var)
 *      bits 1: has_packed_ack? (var)
 *      bits 1: is_cf3_only_cmd? (var)
 *      bits 1: is_server_connected? (var)
 *      bits 1: is_join_allowed? (var)
 *      bits 3: _reserved
 * byte 3:
 *      bits 3: packed_ack_delay_num (~ constant, range [0,7])
 *      bits 2: occupied_capacity (var) \sa enum _gateway_occupied_capacity
 *      bits 2: allowable_max_tx_power (~ constant) \sa lpwan_radio_tx_power_list
 *      bits 1: _reserved
 * byte 4:
 *      bits 8: nearby_channels (~ constant, bit-vector) \sa lpwan_radio_channels_list
 * 
 * \remark
 *      @packed_ack_delay_num is the number of beacon periods to be delayed to
 *      receive packed ack.
 *      If set to 0, the gateway cluster will response immediately.
 */
typedef os_uint8 beacon_info_t[4];

/**
 * byte 1:
 *      bits 8: beacon_seq_id (a lollipop sequence number, range [0,127])
 * byte 2:
 *      bits 4: beacon_group_seq_id (a sequence number, range [0,15])
 *      bits 4: beacon_class_seq_id (a sequence number, range [0, max_class_num])
 */
typedef os_uint8 beacon_seq_t[2];

/**
 * bits 4: ratio_beacon
 * bits 4: ratio_op1
 * bits 4: ratio_op2
 * bits 4: ratio_cf1
 * bits 4: ratio_cf2
 * bits 4: ratio_cf3
 * 
 * \remark We segment the beacon period into 64 sections. 
 *         If beacon period is 2 seconds (i.e. 2000ms), and each second means 
 *         32k (32*1024=32768) clock ticks, so each section is 512 ticks (31.25ms).
 * \remark beacon + op1/2 + cf1/2/3 = 32;
 *         i.e. beacon * 2 is the actual beacon ratio length.
 */
typedef os_uint8 beacon_period_section_ratio_t[3];

__LPWAN struct beacon_header {
    version_t required_min_version;
    version_t lpwan_protocol_version;
    beacon_info_t info;
    beacon_period_section_ratio_t period_ratio;
    beacon_seq_t seq;
};

/**
 * bits 1: is_join_ack?
 * bits 1: is_msg_pending? (besides the ACK, there is also pending msg)
 * bits 3: estimation_down_time
 * bits 2: preferred_next_tx_power (the recommended next tx power of end-device)
 * bits 1: _reserved
 * 
 * \remark
 *      eg. for 2s beacon interval, time = 31.25ms * @estimation_down_time
 *      \sa enum beacon_period_section_ratio_t
 */
typedef os_uint8 packed_ack_header_t;

__LPWAN struct beacon_packed_ack {
    packed_ack_header_t hdr;
    os_uint8 confirmed_seq;
    union {
        short_addr_t short_addr;
        short_modem_uuid_t short_uuid; /**< if @is_join_ack is set in @hdr. */
    } addr;
};

/**
 * bits 1: is_op2_more_pending?
 * bits 7: next_delta_beacon_seq_num
 */
typedef os_uint8 beacon_op2_hdr_t;

/**
 * bits 1: is_more_pending? \sa beacon_op2_hdr_t::is_op2_more_pending
 * bits 1: is_multicast? 
 * bits 3: _reserved
 * bits 3: estimation_down_time
 */
typedef os_uint8 beacon_op2_msg_hdr_t;

__LPWAN struct beacon_op2_msg {
    beacon_op2_msg_hdr_t hdr;
    union {
        short_addr_t short_addr;
        multicast_addr_t multi_addr;
    } addr;
};

/** @} */
/*---------------------------------------------------------------------------*/

/**
 * Beacon enumerations.
 * @{
 */

enum _beacon_period {
    BEACON_PERIOD_2S = 0,       /**< 2*BEACON_MAX_SEQ_NUM = 256s = 4m16s */
    BEACON_PERIOD_3S = 1,       /**< 3*BEACON_MAX_SEQ_NUM = 384s = 6m24s */
    BEACON_PERIOD_4S = 2,       /**< 4*BEACON_MAX_SEQ_NUM = 512s = 8m32s */
    BEACON_PERIOD_5S = 3,       /**< 5*BEACON_MAX_SEQ_NUM = 640s = 10m40s */
    _beacon_period_invalid = 4,
};

/**
 * \brief beacon's max group num.
 * \remark eg. If beacon period is 2s, then max beacon sequence period is
 *         2*(1+0x7F)=2*128=256 seconds (i.e. 4m16s).
 *         4m16s a feasible downlink message "listen-then-sleep-or-receive" 
 *         period, however is not large enough for some uplink message period
 *         (eg. some air quality sensor send message every 30 minutes).
 *         So, we introduce "beacon group".
 *
 * \sa beacon_info_t::beacon_groups_num
 * \sa struct gw_cmd_reserve_op12_confirmed
 */
enum _beacon_max_groups_num {
    BEACON_MAX_GROUP_15 = 0, /**< groupID range from 0~15  \sa BEACON_PERIOD_2S */
    BEACON_MAX_GROUP_9 = 1,  /**< groupID range from 0~10  \sa BEACON_PERIOD_3S */
    BEACON_MAX_GROUP_7  = 2, /**< groupID range from 0~7   \sa BEACON_PERIOD_4S */
    BEACON_MAX_GROUP_5  = 3, /**< groupID range from 0~6   \sa BEACON_PERIOD_5S */
    _beacon_max_group_invalid = 4,
};

/**
 * \brief The capacity that has been occupied by the joined modems.
 */
enum _gateway_occupied_capacity {
    GW_OCCUPIED_CAPACITY_BELOW_25  = 0, /**< 0~25% */
    GW_OCCUPIED_CAPACITY_BELOW_50  = 1, /**< 25~50% */
    GW_OCCUPIED_CAPACITY_BELOW_75  = 2, /**< 50~75% */
    GW_OCCUPIED_CAPACITY_BELOW_100 = 3, /**< 75~100% */ 
    _gateway_occupied_capacity_invalid = 4,
};

/** @} */

#ifdef __GNUC__
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* GW_BEACON_H_ */
