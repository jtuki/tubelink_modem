/**
 * parse_device_uplink_common.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lib/assert.h"
#include "parsed_common_defs.h"
#include "parse_device_uplink_common.h"

__LPWAN void lpwan_parse_device_uplink_common(const struct device_uplink_common *up_common,
                                              os_uint8 buf_len,
                                              struct parsed_device_uplink_common *info)
{
    haddock_assert(info && buf_len > sizeof(struct device_uplink_common));

    info->beacon_seq_id = (os_int8) up_common->beacon_seq[0];
    info->beacon_class_seq_id = get_bits(up_common->beacon_seq[1], 3, 0);

    info->beacon_snr = up_common->beacon_snr;
    info->beacon_rssi = up_common->beacon_rssi;

    /** parse the @device_uplink_common_hdr_t */
    info->is_need_ack = get_bits(up_common->hdr, 7, 7);
    info->retransmit_num = get_bits(up_common->hdr, 4, 3);
    info->tx_fail_num = get_bits(up_common->hdr, 2, 0);
}
