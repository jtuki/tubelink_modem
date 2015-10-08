/**
 * construct_frame_hdr.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */


#include "construct_frame_hdr.h"
#include "lib/assert.h"
#include "lib/hdk_utilities.h"

os_int8 construct_device_frame_header(void *frame_buffer, os_uint8 buffer_len,
                                       enum frame_type_end_device type,
                                       const struct lpwan_addr *src,
                                       const struct lpwan_addr *dest,
                                       os_boolean is_mobile,
                                       os_uint8 tx_power_level)
{
    struct frame_header *hdr = frame_buffer;
    os_uint8 _len;

    switch ((int) type) {
    case FTYPE_DEVICE_JOIN        :
        _len = FRAME_HDR_LEN_JOIN;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_MODEM_UUID
                       && dest->type == ADDR_TYPE_SHORT_ADDRESS);
        hdr->dest_and_src.short_uuid.src = src->addr.uuid;
        hdr->dest_and_src.short_uuid.dest = os_hton_u16((os_uint16) dest->addr.short_addr);
        break;
    case FTYPE_DEVICE_DATA_REQUEST:
    case FTYPE_DEVICE_ACK         :
    case FTYPE_DEVICE_CMD         :
    case FTYPE_DEVICE_MSG         :
        _len = FRAME_HDR_LEN_NORMAL;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_SHORT_ADDRESS
                       && dest->type == ADDR_TYPE_SHORT_ADDRESS);
        hdr->dest_and_src.short_short.src = os_hton_u16((os_uint16) src->addr.short_addr);
        hdr->dest_and_src.short_short.dest = os_hton_u16((os_uint16) dest->addr.short_addr);
        break;
    default:
        __should_never_fall_here();
    }

    set_bits(hdr->hdr, 7, 7, DEVICE_END_DEVICE);
    set_bits(hdr->hdr, 6, 6, is_mobile);
    set_bits(hdr->hdr, 5, 4, tx_power_level);
    set_bits(hdr->hdr, 3, 1, type);

    return _len;
}

/**
 * Update the frame header's address information due to gateway change etc.
 */
void update_device_frame_header_addr(struct frame_header *hdr, os_uint8 hdr_len,
                                     enum frame_type_end_device type,
                                     const struct lpwan_addr *src,
                                     const struct lpwan_addr *dest)
{
    switch ((int) type) {
    case FTYPE_DEVICE_JOIN        :
        haddock_assert(hdr_len == FRAME_HDR_LEN_JOIN);
        haddock_assert(src == NULL // cannot change JOIN req's uuid
                       && dest->type == ADDR_TYPE_SHORT_ADDRESS);
        hdr->dest_and_src.short_uuid.dest = os_hton_u16((os_uint16) dest->addr.short_addr);
        break;
    case FTYPE_DEVICE_DATA_REQUEST:
    case FTYPE_DEVICE_ACK         :
    case FTYPE_DEVICE_CMD         :
    case FTYPE_DEVICE_MSG         :
        haddock_assert(hdr_len == FRAME_HDR_LEN_NORMAL);
        haddock_assert(src->type == ADDR_TYPE_SHORT_ADDRESS
                       && dest->type == ADDR_TYPE_SHORT_ADDRESS);
        hdr->dest_and_src.short_short.src = os_hton_u16((os_uint16) src->addr.short_addr);
        hdr->dest_and_src.short_short.dest = os_hton_u16((os_uint16) dest->addr.short_addr);
        break;
    default:
        __should_never_fall_here();
    }
}

os_int8 construct_gateway_frame_header(void *frame_buffer, os_uint8 buffer_len,
                                        enum frame_type_gw type,
                                        const struct lpwan_addr *src,
                                        const struct lpwan_addr *dest,
                                        os_boolean is_multicast_dest)
{
    struct frame_header *hdr = frame_buffer;
    os_uint8 _len;

    switch ((int) type) {
    case FTYPE_GW_JOIN_CONFIRMED  :
        _len = FRAME_HDR_LEN_JOIN;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_SHORT_ADDRESS
                       && dest->type == ADDR_TYPE_MODEM_UUID);
        hdr->dest_and_src.uuid_short.src = os_hton_u16((os_uint16) src->addr.short_addr);
        hdr->dest_and_src.uuid_short.dest = dest->addr.uuid;
        break;
    case FTYPE_GW_JOIN_PENDING_ACK:
        _len = FRAME_HDR_LEN_NORMAL;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_SHORT_ADDRESS
                       && dest->type == ADDR_TYPE_SHORTENED_MODEM_UUID);
        hdr->dest_and_src.suuid_short.src = os_hton_u16((os_uint16) src->addr.short_addr);
        hdr->dest_and_src.suuid_short.dest = os_hton_u16((os_uint16) dest->addr.suuid);
        break;
    case FTYPE_GW_ACK             :
        // ack to a single short address
        haddock_assert(! is_multicast_dest);
        // no break, fall through
    case FTYPE_GW_BEACON          :
        // beacon to a multicast group
        haddock_assert(is_multicast_dest);
        // no break, fall through
    case FTYPE_GW_CMD             :
    case FTYPE_GW_MSG             :
        _len = FRAME_HDR_LEN_NORMAL;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_SHORT_ADDRESS);

        if (is_multicast_dest) {
            haddock_assert(dest->type == ADDR_TYPE_MULTICAST_ADDRESS);
            hdr->dest_and_src.multi_short.dest = os_hton_u16((os_uint16) dest->addr.multi_addr);
            hdr->dest_and_src.multi_short.src = os_hton_u16((os_uint16) src->addr.short_addr);
        } else {
            haddock_assert(dest->type == ADDR_TYPE_SHORT_ADDRESS);
            hdr->dest_and_src.short_short.dest = os_hton_u16((os_uint16) dest->addr.short_addr);
            hdr->dest_and_src.short_short.src = os_hton_u16((os_uint16) src->addr.short_addr);
        }
        break;
    default:
        __should_never_fall_here();
    }

    set_bits(hdr->hdr, 7, 7, DEVICE_GATEWAY);
    set_bits(hdr->hdr, 3, 1, type);

    return _len;
}
