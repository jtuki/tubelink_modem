/**
 * parse_frame_hdr.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "parse_frame_hdr.h"
#include "lib/hdk_utilities.h"

/**
 * \return the actual frame header length. -1 if error occurs.
 */
__LPWAN os_int8 lpwan_parse_frame_header (const struct frame_header *hdr, os_uint8 len,
                                          struct parsed_frame_hdr_info *info)
{
    os_uint8 _len = 0;

    _len += sizeof(hdr->hdr);
    switch (get_bits(hdr->hdr, 7, 7)) {
    case DEVICE_END_DEVICE:
        info->frame_origin_type = DEVICE_END_DEVICE;
        info->info.de.is_mobile = get_bits(hdr->hdr, 6, 6);
        info->info.de.tx_power_level = get_bits(hdr->hdr, 5, 4);
        info->info.de.frame_type = (enum frame_type_end_device) get_bits(hdr->hdr, 3, 1);

        switch ((int) info->info.de.frame_type) {
        case FTYPE_DEVICE_JOIN         :
        case FTYPE_DEVICE_REJOIN       :
            info->dest.type = ADDR_TYPE_SHORT_ADDRESS;
            info->src.type = ADDR_TYPE_MODEM_UUID;
            info->dest.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.short_uuid.dest);
            info->src.addr.uuid = hdr->dest_and_src.short_uuid.src;
            _len += sizeof(short_addr_t) + sizeof(modem_uuid_t);
            break;
        case FTYPE_DEVICE_DATA_REQUEST :
        case FTYPE_DEVICE_ACK          :
        case FTYPE_DEVICE_CMD          :
        case FTYPE_DEVICE_MSG          :
            info->dest.type = ADDR_TYPE_SHORT_ADDRESS;
            info->src.type = ADDR_TYPE_SHORT_ADDRESS;
            info->dest.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.short_short.dest);
            info->src.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.short_short.src);
            _len += sizeof(short_addr_t) + sizeof(short_addr_t);
            break;
        }
        break;
    case DEVICE_GATEWAY:
        info->frame_origin_type = DEVICE_GATEWAY;
        info->info.gw.is_end_of_section = get_bits(hdr->hdr, 6, 6);
        info->info.gw.end_of_beacon_section = \
                (enum _beacon_period_section) get_bits(hdr->hdr, 5, 4);
        info->info.gw.frame_type = (enum frame_type_gw) get_bits(hdr->hdr, 3, 1);
        info->info.gw.is_multicast_dest = get_bits(hdr->hdr, 0, 0);

        switch ((int) info->info.gw.frame_type) {
        case FTYPE_GW_BEACON           :
        case FTYPE_GW_ACK              :
            info->dest.type = ADDR_TYPE_SHORT_ADDRESS;
            info->src.type = ADDR_TYPE_SHORT_ADDRESS;
            info->dest.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.short_short.dest);
            info->src.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.short_short.src);
            _len += sizeof(short_addr_t) + sizeof(short_addr_t);
            break;
        case FTYPE_GW_CMD              :
        case FTYPE_GW_MSG              :
            info->src.type = ADDR_TYPE_SHORT_ADDRESS;
            if (info->info.gw.is_multicast_dest) {
                info->dest.type = ADDR_TYPE_MULTICAST_ADDRESS;
                info->dest.addr.multi_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.multi_short.dest);
                info->src.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.multi_short.src);
                _len += sizeof(short_addr_t) + sizeof(multicast_addr_t);
            } else {
                info->dest.type = ADDR_TYPE_SHORT_ADDRESS;
                info->dest.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.short_short.dest);
                info->src.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.short_short.src);
                _len += sizeof(short_addr_t) + sizeof(short_addr_t);
            }
            break;
        case FTYPE_GW_JOIN_PENDING_ACK :
            info->dest.type = ADDR_TYPE_SHORTENED_MODEM_UUID;
            info->dest.addr.suuid = os_ntoh_u16((os_uint16) hdr->dest_and_src.suuid_short.dest);
            info->src.type = ADDR_TYPE_SHORT_ADDRESS;
            info->src.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.suuid_short.src);
            _len += sizeof(short_addr_t) + sizeof(short_modem_uuid_t);
            break;
        case FTYPE_GW_JOIN_CONFIRMED   :
            info->dest.type = ADDR_TYPE_MODEM_UUID;
            info->dest.addr.uuid = hdr->dest_and_src.uuid_short.dest;
            info->src.type = ADDR_TYPE_SHORT_ADDRESS;
            info->src.addr.short_addr = os_ntoh_u16((os_uint16) hdr->dest_and_src.uuid_short.src);
            _len += sizeof(short_addr_t) + sizeof(modem_uuid_t);
            break;
        }
        break;
    }

    return (len < _len) ? -1:_len;
}
