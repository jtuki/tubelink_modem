/**
 * construct_de_uplink_common.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "parsed_common_defs.h"
#include "lib/assert.h"
#include "construct_de_uplink_common.h"

void update_device_uplink_common(struct device_uplink_common *up_comm,
                                 os_uint8 retrans, os_uint8 tx_fail,
                                 os_int8 bcn_seq_id, os_uint8 bcn_class_seq_id,
                                 os_int16 bcn_rssi, os_int16 bcn_snr)
{
    haddock_assert(retrans < 4); // 2 bits
    haddock_assert(tx_fail < 8); // 3 bits

    /** \ref struct device_uplink_common::hdr */
    set_bits(up_comm->hdr, 4, 0, (retrans << 3) + tx_fail);

    up_comm->beacon_snr = (os_int8) bcn_snr;
    up_comm->beacon_rssi = (os_int8) (bcn_rssi + 30); // we make a drift here to tackel os_int8 overflow.

    os_uint8 *beacon_seq = (os_uint8 *) up_comm->beacon_seq;
    beacon_seq[0] = (os_uint8) bcn_seq_id;
    beacon_seq[1] = bcn_class_seq_id;
}
