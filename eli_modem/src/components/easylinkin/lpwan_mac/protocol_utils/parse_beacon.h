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
    /**< almost constant values @{ */
    version_t required_min_version;
    version_t lpwan_protocol_version;
    
    os_uint8 packed_ack_delay_num;
    os_int8 allowable_max_tx_power;

    enum _beacon_period beacon_period_enum; /**< \sa beacon_period_length_s */
    os_uint8 beacon_period_length_s;        /**< \sa gl_beacon_period_length_list (unit: seconds) */
    os_uint32 beacon_section_length_us;     /**< \sa gl_beacon_section_length_us (unit: us) */

    os_uint8 beacon_classes_num; /**< \sa struct beacon_info_t::beacon_classes_num
                                      range [1, @BEACON_MAX_CLASSES_NUM] */

    short_addr_t gateway_cluster_addr;  /**< \note @gateway_cluster_addr is not included in beacon body itself,
                                              it should be extracted from frame header's src addr component. */
    /**< @} */

    /**< occasionally changed values @{ */
    os_boolean is_server_connected;
    os_boolean is_join_allowed;
    enum _gateway_occupied_capacity occupied_capacity;
    /**< @} */

    /**< constantly changing values @{ */
    os_int8 beacon_seq_id;          /**< range [0, BEACON_MAX_SEQ_NUM] */
    os_uint8 beacon_class_seq_id;   /**< range [1, ::beacon_classes_num] */

    /**
     * \note sum(slots_xyz) == LPWAN_BEACON_PERIOD_SLOTS_NUM
     * \sa beacon_period_section_ratio_t
     * \sa LPWAN_BEACON_PERIOD_SLOTS_NUM
     */
    struct {
        os_uint8 slots_beacon;
        os_uint8 slots_downlink_msg;
        os_uint8 slots_uplink_msg;      /**< \sa DE_MAC_ENGINE_MIN_UPLINK_SLOTS
                                             \sa LPWAN_DE_MIN_TX_RESERVED_SLOTS */
    } ratio;

    /** packed ack related information */
    os_boolean has_packed_ack;
    os_uint8 packed_ack_num;
    /**< @} */
};

/**
 * The end-device parse the packed ack information contained in the beacon
 * to decide whether there is pending ACK/msg to itself.
 *  
 * \sa struct parsed_beacon_info::has_packed_ack 
 */
struct parsed_beacon_packed_ack_to_me {
    os_boolean has_ack; /**< has received ACK to me? */
    os_boolean is_msg_pending;
    
    os_uint8 total_prev_downlink_slots; /**< sum(previous downlink slots) */
    
    os_uint8 confirmed_seq;
    os_uint8 preferred_next_tx_power;
};

/**
 * Check info flags when parsing beacon.
 */
struct parse_beacon_check_info {
    os_boolean is_check_packed_ack; /**< TRUE if some frames are awaiting ACK */
    os_boolean is_check_join_ack;   /**< TRUE if mac_info_get_mac_states() == DE_MAC_STATES_JOINING */
    modem_uuid_t uuid;
    short_modem_uuid_t suuid;   /**< check packed ACK using suuid if @is_check_join_ack == TRUE */
    short_addr_t short_addr;    /**< check packed ACK using short_addr if @is_check_join_ack == FALSE */
};

__LPWAN os_int8 lpwan_parse_beacon (const os_uint8 beacon[], os_uint8 len,
                                    const struct parse_beacon_check_info *check_info,
                                    struct parsed_beacon_info *info,
                                    struct parsed_beacon_packed_ack_to_me *ack);

__LPWAN os_boolean bcn_packed_ack_hdr_is_join(packed_ack_header_t hdr);
__LPWAN void bcn_packed_ack_hdr_set_pending_msg(packed_ack_header_t *hdr,
                                                os_boolean is_msg_pending,
                                                os_uint8 estimation_downlink_slots);

#ifdef __cplusplus
}
#endif

#endif /* PARSE_BEACON_H_ */
