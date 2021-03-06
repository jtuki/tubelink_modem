/**
 * construct_gw_beacon.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "construct_gw_beacon.h"

os_int8 construct_gateway_beacon_header(void *bcn_buffer,
                                        os_uint8 buffer_len,
                                        struct parsed_beacon_info *bcn_info)
{
    struct beacon_header *bcn_hdr = bcn_buffer;

    if (buffer_len < sizeof(struct beacon_header)) {
        return -1;
    }

    bcn_hdr->required_min_version = bcn_info->required_min_version;
    bcn_hdr->lpwan_protocol_version = bcn_info->lpwan_protocol_version;

    // construct the @info
    os_uint8 *info = bcn_hdr->info;  /**< \sa beacon_info_t */

    /** bcn_info[0] @{ */
    set_bits(info[0], 7, 6, bcn_info->beacon_period_enum);
    set_bits(info[0], 3, 0, bcn_info->beacon_classes_num - 1); // largest value is @BEACON_MAX_CLASSES_NUM
    /** @} */

    /** bcn_info[1] @{ */
    set_bits(info[1], 6, 6, bcn_info->has_packed_ack);
    set_bits(info[1], 5, 5, bcn_info->is_server_connected);
    set_bits(info[1], 4, 4, bcn_info->is_join_allowed);
    /** @} */

    /** bcn_info[2] @{ */
    set_bits(info[2], 7, 5, bcn_info->packed_ack_delay_num);
    set_bits(info[2], 4, 3, bcn_info->occupied_capacity);
    set_bits(info[2], 2, 1, bcn_info->allowable_max_tx_power);
    /** @} */

    /** bcn_info[3] @{ */
    set_bits(info[3], 7, 0, 0); // reserved byte
    /** @} */

    // construct the @period_ratio
    os_uint8 *bcn_ratio = bcn_hdr->period_ratio;

    set_bits(bcn_ratio[0], 7, 0, bcn_info->ratio.slots_beacon);
    set_bits(bcn_ratio[1], 7, 0, bcn_info->ratio.slots_downlink_msg);
    set_bits(bcn_ratio[2], 7, 0, bcn_info->ratio.slots_uplink_msg);

    // construct the @seq
    os_uint8 *bcn_seq_id = bcn_hdr->seq;
    bcn_seq_id[0] = (os_uint8) bcn_info->beacon_seq_id;
    // range [0, info->beacon_classes_num)
    set_bits(bcn_seq_id[1], 3, 0, bcn_info->beacon_class_seq_id);

    return (os_int8) sizeof(struct beacon_header);
}
