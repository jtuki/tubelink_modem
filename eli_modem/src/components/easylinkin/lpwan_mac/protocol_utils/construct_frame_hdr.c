/**
 * construct_frame_hdr.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */


#include "construct_frame_hdr.h"
#include "lib/assert.h"

os_int8 construct_device_frame_header(void *frame_buffer, os_uint8 buffer_len,
                                       enum frame_type_end_device type,
                                       struct lpwan_addr *src,
                                       struct lpwan_addr *dest,
                                       os_boolean is_mobile,
                                       os_uint8 tx_power_level)
{
    struct frame_header *hdr = frame_buffer;
    os_uint8 _len;

    switch ((int) type) {
    case FTYPE_DEVICE_JOIN        :
    case FTYPE_DEVICE_REJOIN      :
        _len = FRAME_HDR_LEN_JOIN;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_MODEM_UUID
                       && dest->type == ADDR_TYPE_SHORT_ADDRESS);
        hdr->dest_and_src.short_uuid.src = src->addr.uuid;
        hdr->dest_and_src.short_uuid.dest = dest->addr.short_addr;
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
        hdr->dest_and_src.short_short.src = src->addr.short_addr;
        hdr->dest_and_src.short_short.dest = dest->addr.short_addr;
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

os_int8 construct_gateway_frame_header(void *frame_buffer, os_uint8 buffer_len,
                                        enum frame_type_gw type,
                                        struct lpwan_addr *src,
                                        struct lpwan_addr *dest,
                                        os_boolean is_multicast_dest,
                                        os_boolean is_end_of_section,
                                        enum _beacon_period_section end_of_beacon_section)
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
        hdr->dest_and_src.uuid_short.src = src->addr.short_addr;
        hdr->dest_and_src.uuid_short.dest = dest->addr.uuid;
        break;
    case FTYPE_GW_JOIN_PENDING_ACK:
        _len = FRAME_HDR_LEN_NORMAL;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_SHORT_ADDRESS
                       && dest->type == ADDR_TYPE_SHORTENED_MODEM_UUID);
        hdr->dest_and_src.suuid_short.src = src->addr.short_addr;
        hdr->dest_and_src.suuid_short.dest = dest->addr.suuid;
        break;
    case FTYPE_GW_ACK             :
        // ack to a single short address
        haddock_assert(! is_multicast_dest);
    case FTYPE_GW_BEACON          :
        // beacon to a multicast group
        haddock_assert(is_multicast_dest);
    case FTYPE_GW_CMD             :
    case FTYPE_GW_MSG             :
        _len = FRAME_HDR_LEN_NORMAL;
        if (buffer_len < _len)
            return -1;
        haddock_assert(src->type == ADDR_TYPE_SHORT_ADDRESS);

        if (is_multicast_dest) {
            haddock_assert(dest->type == ADDR_TYPE_MULTICAST_ADDRESS);
            hdr->dest_and_src.multi_short.dest = dest->addr.multi_addr;
            hdr->dest_and_src.multi_short.src = src->addr.short_addr;
        } else {
            haddock_assert(dest->type == ADDR_TYPE_SHORT_ADDRESS);
            hdr->dest_and_src.short_short.dest = dest->addr.short_addr;
            hdr->dest_and_src.short_short.src = src->addr.short_addr;
        }
        break;
    default:
        __should_never_fall_here();
    }

    set_bits(hdr->hdr, 7, 7, DEVICE_GATEWAY);
    set_bits(hdr->hdr, 6, 6, is_end_of_section);
    set_bits(hdr->hdr, 5, 4, end_of_beacon_section);
    set_bits(hdr->hdr, 3, 1, type);

    return _len;
}
