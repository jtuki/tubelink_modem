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
    
    os_int8 beacon_seq_id; /**< \sa BEACON_MAX_SEQ_NUM */
    os_uint8 beacon_group_seq_id;
    os_uint8 beacon_class_seq_id; /**< range [1, ::beacon_classes_num] */
    
    os_uint8 packed_ack_delay_num;
    
    os_boolean has_op2;
    os_boolean has_packed_ack;
    os_boolean is_cf3_only_cmd;
    
    os_int8 allowable_max_tx_power;
    
    /**
     * \remark The length of each section differs according to
     *         @beacon_period_length.
     * \remark sum(ratio_xxx) == 64
     * \sa beacon_period_section_ratio_t
     */
    struct {
        os_uint8 ratio_beacon;
        os_uint8 ratio_op1;
        os_uint8 ratio_op2;
        os_uint8 ratio_cf1;
        os_uint8 ratio_cf2;
        os_uint8 ratio_cf3;
    } ratio;
    
    enum _beacon_period _beacon_period_length;
    enum _beacon_max_groups_num _beacon_max_groups_num;

    /**< parsed information from @_beacon_period_length and @_beacon_max_groups_num
     * @{ */
    os_uint32 beacon_section_length_us;    /**< \sa _beacon_section_length_us
                                                \sa enum _beacon_period */
    os_uint8 beacon_period_length;     /**< \sa enum _beacon_period */
    os_uint8 beacon_groups_num;        /**< \sa enum _beacon_max_groups_num */
    /**< @} */

    os_uint8 beacon_classes_num; /**< \sa struct beacon_info_t::beacon_classes_num, range [1, 16] */

    os_boolean is_server_connected;
    os_boolean is_join_allowed;
    enum _gateway_occupied_capacity occupied_capacity;

    os_uint8 nearby_channels;

    short_addr_t gateway_cluster_addr;

    // op2 related information
    os_boolean is_op2_more_pending;
    os_int8 next_delta_beacon_seq_num;
    os_uint8 op2_msg_num;

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
    
    /**< If set to 0, listen through the BEACON period, all the time. */
    os_uint8 total_estimation_down_time;
    
    os_uint8 confirmed_seq;
    os_uint8 preferred_next_tx_power;
};

/**
 * \sa struct parsed_beacon_info::has_op2 
 */
struct parsed_beacon_op2_to_me {
    os_boolean has_multicast_reserve;
    os_boolean has_unicast_reserve;
    
    os_uint8 total_multi_estimation_down_time;    
    os_uint8 total_uni_estimation_down_time;
    
    /**
     * \sa struct parsed_beacon_info::is_op2_more_pending
     * \sa struct parsed_beacon_info::next_delta_beacon_seq_num
     */
    os_boolean is_more_pending;
};

struct parse_beacon_check_op2_ack_info {
    os_boolean is_check_op2;
    os_boolean is_check_packed_ack;
    short_addr_t short_addr;
    modem_uuid_t uuid;
    short_modem_uuid_t suuid;
};

__LPWAN os_int8 lpwan_parse_beacon (const os_uint8 beacon[], os_uint8 len,
                                    struct parse_beacon_check_op2_ack_info *check_info,
                                    struct parsed_beacon_info *info,
                                    struct parsed_beacon_packed_ack_to_me *ack,
                                    struct parsed_beacon_op2_to_me *op2);

/** \sa enum _beacon_period */
extern const os_uint8 _beacon_period_length_list[4];
/** \sa enum _beacon_max_groups_num */
extern const os_uint8 _beacon_groups_num_list[4];

#ifdef __cplusplus
}
#endif

#endif /* PARSE_BEACON_H_ */
