/**
 * construct_de_uplink_msg.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "parsed_common_defs.h"
#include "frame_defs/device_uplink_msg.h"

#include "kernel/hdk_memory.h"
#include "lib/assert.h"

#include "lpwan_config.h"

#include "construct_de_uplink_common.h"
#include "construct_de_uplink_msg.h"

/**
 * \sa enum device_message_type
 */
const char *device_msg_type_string[3] = {"m_r", "m_e", "m_em"};

/**
 * Construct device uplink message:
 *      type, len, msg content
 * \note all messages require the packed ACK from gateway.
 */
void construct_device_uplink_msg(enum device_message_type type,
                                 const os_uint8 msg[], os_uint8 len,
                                 void *buffer, os_uint8 buffer_len)
{
    haddock_assert(len < LPWAN_MAX_PAYLAOD_LEN);
    haddock_assert(buffer_len > sizeof(struct device_uplink_msg) + len);

    struct device_uplink_msg *up = (struct device_uplink_msg *) buffer;

    up->type_and_len = (((os_uint8) type) << 6) + len;
    haddock_memcpy(up->msg, msg, len);

    // all messages require the packed ACK from gateway.
    set_bits(((os_uint8) up->hdr.hdr), 7, 7, OS_TRUE);
}

/**
 * Update the device's uplink message (emergent/event/routine).
 * \note @retrans and @tx_fail are the retransmission times and tx fail
 *       times of the @msg.
 */
void update_device_uplink_msg(struct device_uplink_msg *msg,
                              os_uint8 frame_seq_id,
                              os_uint8 retrans, os_uint8 tx_fail,
                              os_int8 bcn_seq_id, os_uint8 bcn_class_seq_id,
                              os_int16 bcn_rssi, os_int16 bcn_snr)
{
    haddock_assert(bcn_class_seq_id <= 15);
    msg->seq = frame_seq_id;

    update_device_uplink_common(& msg->hdr,
                                retrans, tx_fail,
                                bcn_seq_id, bcn_class_seq_id,
                                bcn_rssi, bcn_snr);
}
