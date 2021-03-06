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
#include "lpwan_config.h"

struct beacon_frame_offsets {
    os_uint8 offset_packed_ack_num;
    os_uint8 total_frame_len;
};

static void expected_beacon_length(const os_uint8 beacon[],
                                   struct beacon_frame_offsets *offsets);

__LPWAN os_int8 lpwan_parse_beacon (const os_uint8 beacon[], os_uint8 len,
                                    const struct parse_beacon_check_info *check_info,
                                    struct parsed_beacon_info *bcn,
                                    struct parsed_beacon_packed_ack_to_me *ack)
{
    static struct beacon_frame_offsets bcn_offsets;
    expected_beacon_length(beacon, & bcn_offsets);
    if (len != bcn_offsets.total_frame_len)
        return -1;

    const struct beacon_header *bcn_hdr = (const struct beacon_header *) beacon;
    bcn->required_min_version = bcn_hdr->required_min_version;
    bcn->lpwan_protocol_version = bcn_hdr->lpwan_protocol_version;

    const os_uint8 *bcn_info = bcn_hdr->info;  /**< \sa beacon_info_t */

    /** bcn_info[0] @{ */
    switch ((os_uint8) get_bits(bcn_info[0], 7, 6)) {
    case BEACON_PERIOD_2S:
    case BEACON_PERIOD_4S:
    case BEACON_PERIOD_8S:
    case BEACON_PERIOD_16S:
        bcn->beacon_period_length_s = \
            gl_beacon_period_length_list[(int) get_bits(bcn_info[0], 7, 6)];
        bcn->beacon_section_length_us = \
            gl_beacon_section_length_us[(int) get_bits(bcn_info[0], 7, 6)];
        break;
    }

    bcn->beacon_classes_num = get_bits(bcn_info[0], 3, 0) + 1; // highest value is @BEACON_MAX_CLASSES_NUM
    /** @} */

    /** bcn_info[1] @{ */
    bcn->has_packed_ack        = get_bits(bcn_info[1], 6, 6);
    bcn->is_server_connected   = get_bits(bcn_info[1], 5, 5);
    bcn->is_join_allowed       = get_bits(bcn_info[1], 4, 4);
    /** @} */

    /** bcn_info[2] @{ */
    bcn->packed_ack_delay_num   = get_bits(bcn_info[2], 7, 5);
    bcn->occupied_capacity      = \
            (enum _gateway_occupied_capacity) get_bits(bcn_info[2], 4, 3);
    bcn->allowable_max_tx_power = get_bits(bcn_info[2], 2, 1);
    /** @} */

    const os_uint8 *bcn_ratio = bcn_hdr->period_ratio;

    bcn->ratio.slots_beacon        = get_bits(bcn_ratio[0], 7, 0);
    bcn->ratio.slots_downlink_msg  = get_bits(bcn_ratio[1], 7, 0);
    bcn->ratio.slots_uplink_msg    = get_bits(bcn_ratio[2], 7, 0);
    if (LPWAN_BEACON_PERIOD_SLOTS_NUM !=
        bcn->ratio.slots_beacon +
        bcn->ratio.slots_downlink_msg + bcn->ratio.slots_uplink_msg)
        return -1;

    const os_uint8 *bcn_seq_id = bcn_hdr->seq;
    bcn->beacon_seq_id         = (os_int8) bcn_seq_id[0];
    // range [0, info->beacon_classes_num)
    bcn->beacon_class_seq_id   = get_bits(bcn_seq_id[1], 3, 0);

    /**< check the packed ACK information. @{ */
    ack->has_ack = OS_FALSE;
    ack->is_msg_pending = OS_FALSE;

    static struct beacon_packed_ack *cur_ack;
    if (check_info->is_check_packed_ack && bcn->has_packed_ack) {
        os_uint8 _offset = bcn_offsets.offset_packed_ack_num;
        os_uint8 _packed_ack_num = beacon[_offset];
        _offset += sizeof(os_uint8); // move to the struct beacon_packed_ack(s)

        short_modem_uuid_t check_suuid = \
                (short_modem_uuid_t) os_hton_u16((os_uint16) check_info->suuid);
        short_addr_t check_short_addr = \
                (short_addr_t) os_hton_u16((os_uint16) check_info->short_addr);

        /*
         * the information stored in @cur_ack->hdr
         */
        os_boolean _is_join_ack;
        os_boolean _is_msg_pending;
        os_uint8 _downlink_slots;
        os_uint8 _preferred_tx_power;

        os_uint8 _total_prev_downlink_slots = 0;

        for (os_uint8 i=0; i < _packed_ack_num; ++i) {
            cur_ack = (struct beacon_packed_ack *) & beacon[_offset];

            _is_join_ack        = get_bits(cur_ack->hdr, 7, 7);
            _is_msg_pending     = get_bits(cur_ack->hdr, 6, 6);
            _downlink_slots     = get_bits(cur_ack->hdr, 5, 3);
            _preferred_tx_power = get_bits(cur_ack->hdr, 2, 1);

            if ((check_info->is_check_join_ack
                 && _is_join_ack && cur_ack->addr.short_uuid == check_suuid)
                ||
                (!check_info->is_check_join_ack
                 && !_is_join_ack && cur_ack->addr.short_addr == check_short_addr))
            {
                /**
                 * \remark
                 *  Before joined, the end device has only uuid and suuid.
                 *  The @uuid is unique, and @suuid (calculated from @uuid) is not.
                 *  We only find the first matched @suuid, and if this is not the
                 *  unique @suuid within this BEACON period, the end device will
                 *  listen all the ack through the BEACON period.
                 */
                ack->has_ack = OS_TRUE;
                ack->total_prev_downlink_slots = _total_prev_downlink_slots;
                ack->preferred_next_tx_power = _preferred_tx_power;
                ack->is_msg_pending = _is_msg_pending;
                ack->confirmed_seq = cur_ack->confirmed_seq;

                // found a matching item, exit loop
                break;
            }

            _total_prev_downlink_slots += _downlink_slots;
            _offset += sizeof(struct beacon_packed_ack);
        }
    }
    /**< @} */

    return len;
}

__LPWAN
os_boolean bcn_packed_ack_hdr_is_join(packed_ack_header_t hdr)
{
    return (os_boolean) get_bits(hdr, 7, 7);
}

__LPWAN
void bcn_packed_ack_hdr_set_pending_msg(packed_ack_header_t *hdr,
                                        os_boolean is_msg_pending,
                                        os_uint8 estimation_downlink_slots)
{
    set_bits(*hdr, 6, 6, is_msg_pending);
    set_bits(*hdr, 5, 3, estimation_downlink_slots);
}

static void expected_beacon_length(const os_uint8 beacon[],
                                   struct beacon_frame_offsets *offsets)
{
    os_uint8 len = sizeof(struct beacon_header);

    const struct beacon_header *bcn_hdr = (const struct beacon_header *) beacon;
    const os_uint8 *bcn_info = bcn_hdr->info;  /**< \sa beacon_info_t */

    os_boolean has_packed_ack   = get_bits(bcn_info[1], 6, 6);

    offsets->offset_packed_ack_num = 0;

    if (has_packed_ack) {
        offsets->offset_packed_ack_num = len;

        os_uint8 packed_ack_num = beacon[len];
        len += sizeof(os_uint8) + packed_ack_num * sizeof(struct beacon_packed_ack);
    }

    offsets->total_frame_len = len;
}
