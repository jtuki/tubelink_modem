/**
 * common_defs.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef COMMON_DEFS_H_
#define COMMON_DEFS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * uint8 majorVersion  :5;
 * uint8 minorVersion  :3;
 */
typedef os_uint8 version_t;

typedef os_uint16 short_addr_t;
typedef os_uint16 multicast_addr_t;

#define LPWAN_SYS_ADDR_MULTICAST_ALL    0xFFFF
#define LPWAN_SYS_ADDR_MULTICAST_GROUP  0xFFFC

typedef os_uint32 app_id_t; /**< \sa multicast_addr_t */

typedef struct {
    os_uint8 addr[12];
} modem_uuid_t;

typedef os_uint16 short_modem_uuid_t; /**< used for ack \sa short_modem_uuid */
#define SHORT_MODEM_UUID(uuid) short_modem_uuid(uuid)

typedef union {
    short_addr_t short_addr;
    short_modem_uuid_t suuid;
    multicast_addr_t multi_addr;
} modem_short_t;

/**
 * A random-generated 16-bits value.
 */
typedef os_uint16 token_code_t;

enum _device_type {
    DEVICE_END_DEVICE = 0,
    DEVICE_GATEWAY,
    _device_type_invalid = 2,
};

enum _address_type {
    ADDRESS_TYPE_SHORT = 0,     /**< 2 bytes. \sa short_addr_t, multicast_addr_t */
    ADDRESS_TYPE_UUID,          /**< 12 bytes. \sa modem_uuid_t */
    _address_type_invalid = 2,
};

/**
 * As for beacon, gateway command / message, the frame's source and destination
 * address are 2 bytes long.
 * Only destination address might be multicast address.
 * For most message sent by gateway, the @src is the gateway's cluster id.
 *
 * As for join confirmed, the destination address are 12 bytes
 * modem_uuid_t.
 *
 * \sa struct frame_header::hdr::frame_type
 */
enum frame_type_gw {
    FTYPE_GW_BEACON             = 0,
    FTYPE_GW_ACK                = 1,    // no use currently
    FTYPE_GW_JOIN_PENDING_ACK   = 2,    // no use currently
    FTYPE_GW_JOIN_CONFIRMED     = 3,
    FTYPE_GW_CMD                = 4,    // no use currently
    FTYPE_GW_MSG                = 5,
    _ftype_gw_invalid = 8,
};

/**
 * As for join and rejoin, the source address are modem_uuid_t (12 bytes long).
 * All the destination address _should_ be 2 bytes short_addr_t.
 *
 * \remark It's not allowed to send directly to other end-devices using
 * modem_uuid_t.
 *
 * \sa struct frame_header::hdr::frame_type
 */
enum frame_type_end_device {
    FTYPE_DEVICE_JOIN           = 0,
    FTYPE_DEVICE_DATA_REQUEST   = 1,
    FTYPE_DEVICE_ACK            = 2,    /**< for downlink message acknowledgement. */
    FTYPE_DEVICE_CMD            = 3,    // no use currently
    FTYPE_DEVICE_MSG            = 4,
    _ftype_device_invalid = 8,
};

enum device_message_type {
    DEVICE_MSG_TYPE_ROUTINE     = 0,
    DEVICE_MSG_TYPE_EVENT       = 1,
    DEVICE_MSG_TYPE_EMERGENT    = 2,
    _device_msg_type_invalid = 4,
};

enum cmd_id_end_device {
    MODEM_CMD_ID_LEAVE          = 0,
    _cmd_id_end_device_invalid  = 16,
};

enum cmd_id_gw {
    GW_CMD_ID_FORCE_LEAVE       = 0,
    _gw_cmd_id_invalid          = 16,
};

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* COMMON_DEFS_H_ */
