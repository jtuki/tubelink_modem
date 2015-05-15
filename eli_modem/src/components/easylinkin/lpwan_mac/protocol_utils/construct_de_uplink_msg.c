/**
 * construct_de_uplink_msg.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "frame_defs/device_uplink_msg.h"
#include "construct_de_uplink_msg.h"
#include "kernel/hdk_memory.h"
#include "lib/assert.h"
#include "lpwan_config.h"

/**
 * We only deal with @type_and_len and @msg in stuct device_uplink_msg.
 * All other parts will be updated in other function.
 * todo (we don't care @is_need_ack and @contain_multicast_ack etc.)
 *
 * \sa update_device_tx_frame_msg() in \file mac_engine.c
 */
void construct_device_uplink_msg(enum device_message_type type,
                                 const os_uint8 msg[], os_uint8 msg_len,
                                 void *buffer, os_uint8 buffer_len)
{
    haddock_assert(msg_len < LPWAN_MAX_PAYLAOD_LEN);

    struct device_uplink_msg *up = (struct device_uplink_msg *) buffer;

    up->type_and_len = (((os_uint8) type) << 6) + msg_len;
    haddock_memcpy(up->msg, msg, msg_len);
}
