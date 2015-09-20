/**
 * parse_beacon.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PARSE_BEACON_H_
#define PARSE_BEACON_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "parsed_common_defs.h"

/**< \sa struct beacon_header */
struct parsed_beacon_info {
    version_t required_min_version;
    version_t lpwan_protocol_version;
    
    os_int8 beacon_seq_id; /**< range [0, BEACON_MAX_SEQ_NUM] */
    os_uint8 beacon_class_seq_id; /**< range [1, ::beacon_classes_num] */
    
    os_uint8 packed_ack_delay_num;
    
    os_boolean has_packed_ack;
    
    os_int8 allowable_max_tx_power;
    
    /**
     * \remark The length of each section differs according to
     *         @beacon_period_length.
     * \remark sum(ratio_xxx) == 128 (BEACON_PERIOD_SECTIONS_NUM)
     * \sa beacon_period_section_ratio_t
     * \sa BEACON_PERIOD_SECTIONS_NUM
     */
    struct {
        os_uint8 ratio_beacon;
        os_uint8 ratio_downlink_msg;
        os_uint8 ratio_uplink_msg;      /**< \sa DEVICE_MAC_MIN_CF_RATIO */
    } ratio;
    
    enum _beacon_period _beacon_period_length;

    /**< parsed information from @_beacon_period_length.
     * @{ */
    os_uint32 beacon_section_length_us;    /**< \sa _beacon_section_length_us
                                                \sa enum _beacon_period */
    os_uint8 beacon_period_length;     /**< beacon period in seconds. \sa enum _beacon_period */
    /**< @} */

    os_uint8 beacon_classes_num; /**< \sa struct beacon_info_t::beacon_classes_num
                                      range [1, @BEACON_MAX_CLASSES_NUM] */

    os_boolean is_server_connected;
    os_boolean is_join_allowed;
    enum _gateway_occupied_capacity occupied_capacity;

    os_uint8 nearby_channels;

    short_addr_t gateway_cluster_addr;

    // packed ack related information
    os_uint8 packed_ack_num;
};

/**
 * The end-device parse the packed ack information contained in the beacon
 * to decide whether there is pending ACK/msg to itself.
 *  
 * \sa struct parsed_beacon_info::has_packed_ack 
 */
struct parsed_beacon_packed_ack_to_me {
    os_boolean has_ack; /**< has received ACK? */
    os_boolean is_msg_pending;
    
    os_uint8 total_estimation_downlink_slots;
    
    os_uint8 confirmed_seq;
    os_uint8 preferred_next_tx_power;
};

struct parse_beacon_check_info {
    os_boolean is_check_packed_ack;
    os_boolean is_check_join_ack;   // if is joining, we only check join ACK.
    short_addr_t short_addr;
    modem_uuid_t uuid;
    short_modem_uuid_t suuid;
};

__LPWAN os_int8 lpwan_parse_beacon (const os_uint8 beacon[], os_uint8 len,
                                    struct parse_beacon_check_info *check_info,
                                    struct parsed_beacon_info *info,
                                    struct parsed_beacon_packed_ack_to_me *ack);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_BEACON_H_ */
