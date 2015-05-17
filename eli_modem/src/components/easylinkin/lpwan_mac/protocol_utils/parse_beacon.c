/**
 * parse_beacon.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lpwan_mac/end_device/mac_engine.h"
#include "frame_defs/gw_beacon.h"
#include "parse_beacon.h"

/** \sa enum _beacon_period */
const os_uint8 _beacon_period_length_list[4] = {2, 3, 4, 5};

/** \sa enum _beacon_max_groups_num */
const os_uint8 _beacon_groups_num_list[4] = {15, 9, 7, 5};

/** We segment each beacon period into 64 sections. Below are the section length
 *  for different beacon period length. (uint: us)
 * \sa beacon_period_section_ratio_t */
static const os_uint32 _beacon_section_length_us[] = {31250, 46875, 62500, 78125};

struct op2_ack_offsets {
    os_uint8 total_len;
    os_uint8 offset_op2_hdr;
    os_uint8 offset_packed_ack_num;
};

static void expected_beacon_length(const os_uint8 beacon[],
                                   struct op2_ack_offsets *offsets);

__LPWAN os_int8 lpwan_parse_beacon (const os_uint8 beacon[], os_uint8 len,
                                    struct parse_beacon_check_op2_ack_info *check_info,
                                    struct parsed_beacon_info *info,
                                    struct parsed_beacon_packed_ack_to_me *ack,
                                    struct parsed_beacon_op2_to_me *op2)
{
    static struct op2_ack_offsets bcn_offsets;
    expected_beacon_length(beacon, & bcn_offsets);
    if (len != bcn_offsets.total_len)
        return -1;

    const struct beacon_header *bcn_hdr = (const struct beacon_header *) beacon;
    info->required_min_version = bcn_hdr->required_min_version;
    info->lpwan_protocol_version = bcn_hdr->lpwan_protocol_version;

    const os_uint8 *bcn_info = bcn_hdr->info;  /**< \sa beacon_info_t */

    /** bcn_info[0] @{ */
    switch ((os_uint8) get_bits(bcn_info[0], 7, 6)) {
    case BEACON_PERIOD_2S:
    case BEACON_PERIOD_3S:
    case BEACON_PERIOD_4S:
    case BEACON_PERIOD_5S:
        info->beacon_period_length = \
            _beacon_period_length_list[(int) get_bits(bcn_info[0], 7, 6)];
        info->beacon_section_length_us = \
            _beacon_section_length_us[(int) get_bits(bcn_info[0], 7, 6)];
        break;
    }

    switch ((os_uint8) get_bits(bcn_info[0], 5, 4)) {
    case BEACON_MAX_GROUP_15:
    case BEACON_MAX_GROUP_9:
    case BEACON_MAX_GROUP_7:
    case BEACON_MAX_GROUP_5:
        info->beacon_groups_num = \
            _beacon_groups_num_list[(int) get_bits(bcn_info[0], 5, 4)];
        break;
    }

    info->beacon_classes_num = get_bits(bcn_info[0], 3, 0) + 1; // highest value is 16
    /** @} */

    /** bcn_info[1] @{ */
    info->has_op2               = get_bits(bcn_info[1], 7, 7);
    info->has_packed_ack        = get_bits(bcn_info[1], 6, 6);
    info->is_cf3_only_cmd       = get_bits(bcn_info[1], 5, 5);
    info->is_server_connected   = get_bits(bcn_info[1], 4, 4);
    info->is_join_allowed       = get_bits(bcn_info[1], 3, 3);
    /** @} */

    /** bcn_info[2] @{ */
    info->packed_ack_delay_num   = get_bits(bcn_info[2], 7, 5);
    info->occupied_capacity      = \
            (enum _gateway_occupied_capacity) get_bits(bcn_info[2], 4, 3);
    info->allowable_max_tx_power = get_bits(bcn_info[2], 2, 1);
    /** @} */

    /** bcn_info[3] @{ */
    info->nearby_channels = bcn_info[3];
    /** @} */

    const os_uint8 *bcn_ratio = bcn_hdr->period_ratio;

    info->ratio.ratio_beacon = get_bits(bcn_ratio[0], 7, 4);
    info->ratio.ratio_op1    = get_bits(bcn_ratio[0], 3, 0);
    info->ratio.ratio_op2    = get_bits(bcn_ratio[1], 7, 4);
    info->ratio.ratio_cf1    = get_bits(bcn_ratio[1], 3, 0);
    info->ratio.ratio_cf2    = get_bits(bcn_ratio[2], 7, 4);
    info->ratio.ratio_cf3    = get_bits(bcn_ratio[2], 3, 0);

    const os_uint8 *bcn_seq_id = bcn_hdr->seq;
    info->beacon_seq_id         = (os_int8) bcn_seq_id[0];
    info->beacon_group_seq_id   = get_bits(bcn_seq_id[1], 7, 4);
    // range [1, info->beacon_classes_num]
    info->beacon_class_seq_id   = get_bits(bcn_seq_id[1], 3, 0) + 1;

    if (check_info->is_check_op2 && info->has_op2) {
        // todo
    }

    static struct beacon_packed_ack *cur_ack;
    if (check_info->is_check_packed_ack && info->has_packed_ack) {
        ack->has_ack = OS_FALSE;

        os_uint8 _offset = bcn_offsets.offset_packed_ack_num;
        os_uint8 _packed_ack_num = beacon[_offset];
        _offset += sizeof(os_uint8); // move to the struct beacon_packed_ack(s)

        /*
         * the information stored in @cur_ack->hdr
         */
        os_boolean _is_join_ack;
        os_boolean _is_msg_pending;
        os_uint8 _estimatioon_downtime;
        os_uint8 _preferred_tx_power;

        os_uint8 _total_estimation_downtime = 0;

        for (os_uint8 i=0; i < _packed_ack_num; ++i) {
            cur_ack = (struct beacon_packed_ack *) & beacon[_offset];

            _is_join_ack            = get_bits(cur_ack->hdr, 7, 7);
            _is_msg_pending         = get_bits(cur_ack->hdr, 6, 6);
            _estimatioon_downtime   = get_bits(cur_ack->hdr, 5, 3);
            _preferred_tx_power     = get_bits(cur_ack->hdr, 2, 1);

            _total_estimation_downtime += _estimatioon_downtime;

            if ((_is_join_ack && cur_ack->addr.short_uuid == check_info->suuid)
                || (!_is_join_ack && cur_ack->addr.short_addr == check_info->short_addr))
            {
                /**
                 * \remark
                 *  Before joined, the end device has only uuid ans suuid.
                 *  The @uuid is unique, and @suuid is not.
                 *  We only find the first matched @suuid, and if this is not the
                 *  unique @suuid within this BEACON period, the end device will
                 *  listen all the ack through the BEACON period.
                 */
                ack->has_ack = OS_TRUE;
                ack->total_estimation_down_time = _total_estimation_downtime;
                ack->preferred_next_tx_power = _preferred_tx_power;
                ack->is_msg_pending = _is_msg_pending;
                ack->confirmed_seq = cur_ack->confirmed_seq;

                // found a matching item, exit loop
                break;
            }

            _offset += sizeof(struct beacon_packed_ack);
        }
    }

    return len;
}


static void expected_beacon_length(const os_uint8 beacon[],
                                   struct op2_ack_offsets *offsets)
{
    os_uint8 len = sizeof(struct beacon_header);

    const struct beacon_header *bcn_hdr = (const struct beacon_header *) beacon;
    const os_uint8 *bcn_info = bcn_hdr->info;  /**< \sa beacon_info_t */

    os_boolean has_op2          = get_bits(bcn_info[0], 7, 7);
    os_boolean has_packed_ack   = get_bits(bcn_info[0], 6, 6);

    offsets->offset_op2_hdr = offsets->offset_packed_ack_num = 0;

    if (has_op2) {
        offsets->offset_op2_hdr = len;

        os_uint8 op2_msg_num = beacon[len + sizeof(beacon_op2_hdr_t)];
        len += sizeof(os_uint8) + op2_msg_num * sizeof(struct beacon_op2_msg);
    }

    if (has_packed_ack) {
        offsets->offset_packed_ack_num = len;

        os_uint8 packed_ack_num = beacon[len];
        len += sizeof(os_uint8) + packed_ack_num * sizeof(struct beacon_packed_ack);
    }

    offsets->total_len = len;
}
