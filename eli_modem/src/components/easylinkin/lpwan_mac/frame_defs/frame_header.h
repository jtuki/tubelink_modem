/**
 * frame_header.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef FRAME_HEADER_H_
#define FRAME_HEADER_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "common_defs.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * The length of frame headers.
 * For join/rejoin request/response, the frame header's length is a bit longer.
 */
#define FRAME_HDR_LEN_JOIN \
    (sizeof(os_uint8) + sizeof(modem_uuid_t) + sizeof(short_addr_t))

#define FRAME_HDR_LEN_NORMAL \
    (sizeof(os_uint8) + sizeof(short_addr_t) + sizeof(short_addr_t))

__LPWAN struct frame_header {
    /**
     * bits 1: frame_origin_type \sa enum _device_type
     * bits 1:
     *      gw: _reserved
     *      de: is_mobile (for frame_type_end_device)
     * bits 2:
     *      gw: _reserved
     *      de: tx_power_level (for frame_type_end_device \sa lpwan_radio_tx_power_list)
     *
     * bits 3: frame_type \sa enum frame_type_gw, enum frame_type_end_device
     * bits 1: is_multicast_dest?
     */
    os_uint8 hdr;
    /**
     * Destination and source address (@dest and @src).
     * \sa enum frame_type_gw, enum frame_type_end_device
     */
    union {
        struct {
            short_addr_t dest;
            short_addr_t src;
        } short_short;
        struct {
            multicast_addr_t dest;
            short_addr_t src;
        } multi_short;  /**< eg. FTYPE_GW_CMD, FTYPE_GW_MSG, to multicast address */
        struct {
            short_addr_t dest;
            modem_uuid_t src;
        } short_uuid;   /**< eg. FTYPE_DEVICE_JOIN */
        struct {
            modem_uuid_t dest;
            short_addr_t src;
        } uuid_short;   /**< eg. FTYPE_GW_JOIN_CONFIRMED */
        struct {
            short_modem_uuid_t dest;
            short_addr_t src;
        } suuid_short; /**< shortened modem_UUID as dest. eg. FTYPE_GW_JOIN_PENDING_ACK */
    } dest_and_src;
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* FRAME_HEADER_H_ */
