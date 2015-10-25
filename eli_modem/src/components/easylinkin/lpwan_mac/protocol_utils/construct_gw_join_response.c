/**
 * construct_gw_join_response.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lpwan_types.h"

#include "kernel/hdk_memory.h"

#include "construct_frame_hdr.h"
#include "construct_gw_join_response.h"

/**
 * Construct gateway's JOIN response frame (including the frame header).
 * \return -1 if failed to construct JOIN response. length of JOIN_COMFIRM frame if ok.
 */
os_int8 construct_gateway_join_response(void *frame_buffer, os_uint8 buffer_len,
                                        const modem_uuid_t *uuid,
                                        short_addr_t gw_cluster_addr,
                                        const struct gw_join_confirmed *join_response)
{
    os_int8 len;

    struct lpwan_addr src;
    struct lpwan_addr dest;

    src.type = ADDR_TYPE_SHORT_ADDRESS;
    src.addr.short_addr = gw_cluster_addr;
    dest.type = ADDR_TYPE_MODEM_UUID;
    dest.addr.uuid = *uuid;

    len = construct_gateway_frame_header(frame_buffer, buffer_len,
                                         FTYPE_GW_JOIN_CONFIRMED,
                                         &src, &dest, OS_FALSE);
    if (len < 0
        || buffer_len < (len + sizeof(struct gw_join_confirmed)))
        return -1;

    haddock_memcpy(& ((os_uint8 *)frame_buffer)[len], join_response,
                   sizeof(struct gw_join_confirmed));
    len += sizeof(struct gw_join_confirmed);

    return len;
}

/**
 * Construct gateway's downlink msg frame (including the frame header).
 * \return -1 if failed to construct JOIN response. length of JOIN_COMFIRM frame if ok.
 * \note For convenience, we put this function here instead of a new file.
 */
os_int8 construct_gateway_downlink_msg(void *frame_buffer, os_uint8 buffer_len,
                                       short_addr_t sn_short, short_addr_t gw_cluster_addr,
                                       const os_uint8 msg[], os_uint8 msg_len)
{
    os_int8 len;

    struct lpwan_addr src;
    struct lpwan_addr dest;

    src.type = ADDR_TYPE_SHORT_ADDRESS;
    src.addr.short_addr = gw_cluster_addr;
    dest.type = ADDR_TYPE_SHORT_ADDRESS;
    dest.addr.short_addr = sn_short;

    len = construct_gateway_frame_header(frame_buffer, buffer_len,
                                         FTYPE_GW_MSG,
                                         &src, &dest, OS_FALSE);
    if (len < 0
        || buffer_len < (len + 1+ msg_len)) // 1 for gw_downlink_common_hdr_t
        return -1;

    ((os_uint8 *)frame_buffer)[len++] = 0xE0;    // gw_downlink_common_hdr_t (preferred max tx power, need ack)
    haddock_memcpy(& ((os_uint8 *)frame_buffer)[len], msg, msg_len);
    len += msg_len;

    return len;
}
