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
