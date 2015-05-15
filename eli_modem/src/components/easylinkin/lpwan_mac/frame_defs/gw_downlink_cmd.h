/**
 * gw_downlink_cmd.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef GW_DOWNLINK_CMD_H_
#define GW_DOWNLINK_CMD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "gw_downlink_common.h"

#ifdef __GNUC__
#pragma pack(push)
#pragma pack(1)
#endif

/*---------------------------------------------------------------------------*/
/**< gateway cmd: reserve information @{ */

__LPWAN struct gw_downlink_cmd_common {
    gw_downlink_common_hdr_t hdr;
    os_uint8 seq;
};

/**
 * bits 4: cmd_id   \sa GW_CMD_ID_RESERVE_INFO
 * bits 1: is_contain_op1?
 * bits 1: is_contain_op2?
 * bits 1: is_contain_multicast_info?
 * bits 1: _reserved
 */
typedef os_uint8 gw_cmd_reserve_info_hdr_t;

/**
 * bits 2: type (op1 or op2 or multicast? \sa enum _gw_reserve_details_type)
 * bits 2:
 *         reserve_info_type    \sa enum _gw_reserve_info_type
 * bits 1: _reserved
 * bits 3:
 *      (if @reserve_info_type is GW_RESERVE_DENIED)
 *         denied_reason    \sa enum _gw_reserve_denied_reason
 *      (else)
 *         beacon_channel   \sa lpwan_radio_channels_list
 */
typedef os_uint8 gw_cmd_reserve_details_hdr_t;

__LPWAN struct gw_cmd_reserve_info {
    gw_cmd_reserve_info_hdr_t hdr;
    union {
        struct {
            gw_cmd_reserve_details_hdr_t hdr;
        } denied, multicast_not_found;
        struct {
            gw_cmd_reserve_details_hdr_t hdr;
            os_int8 beacon_seq;
            os_uint16 beacon_group_bit_vector; /**< \sa enum _beacon_max_groups_num */
        } canceled, confirmed, transfer, multicast_found;
    } details;
};

enum _gw_reserve_details_type {
    GW_RESERVE_DETAILS_OP1 = 0,
    GW_RESERVE_DETAILS_OP2,
    GW_RESERVE_DETAILS_MULTICAST,
    _gw_reserve_details_type_invalid = 4,
};

enum _gw_reserve_info_type {
    /**< for op1 and op2 @{ */
    GW_RESERVE_CONFIRMED   = 0,
    GW_RESERVE_CANCELED    = 1,
    GW_RESERVE_DENIED      = 2,
    GW_RESERVE_TRANSFER    = 3, /**< for future multi-channel support */
    _gw_reserve_info_type_op12_invalid = 4,
    /**< @} */
    /**< for multicast @{ */
    GW_RESERVE_MULTICAST_FOUND = 0,
    GW_RESERVE_MULTICAST_NOT_FOUND,
    _gw_reserve_info_type_multicast_invalid = 4,
    /**< @} */
};

enum _gw_reserve_denied_reason {
    GW_RESERVE_DENIED_OUT_OF_FEE = 0,
    GW_RESERVE_DENIED_CANCEL_FIRST,
    _gw_reserve_denied_reason_invalid = 8,
};

/**< @} */
/*---------------------------------------------------------------------------*/
/**< gateway cmd: force leave @{ */

__LPWAN struct gw_cmd_force_leave {
    /**
     * bits 4: cmd_id   \sa GW_CMD_ID_FORCE_LEAVE
     * bits 4: force_leave_reason   \sa enum _gw_cmd_force_leave_reason
     */
    os_uint8 cmd_info;
};

/**
 * \brief The reason for gateway to notify modems to leave.
 *
 * @GW_FORCE_LEAVE_REASON_OUT_OF_FEE
 *      The modem has run out of subscription fee. If a modem receives this
 *      command, it is not allowed to send any emergent/event message, and all
 *      the reserved OP1/2 sections are freed; and the modem _should_ poll the
 *      gateway for downlink join confirmed message with @JOIN_CONFIRMED_PAID
 *      is set (when the subscription fee is paid).
 *      \sa enum _join_confirmed_info
 *
 * @GW_FORCE_LEAVE_REASON_OUT_OF_FEE_LEAVE
 *      With respect to @GW_FORCE_LEAVE_REASON_OUT_OF_FEE, the modem don't need
 *      to actually leave the network, nor the gateway delete the record which the
 *      modem holds. However, @GW_FORCE_LEAVE_REASON_OUT_OF_FEE_LEAVE will force
 *      the modem to leave the network (maybe due to capacity is almost full etc.).
 *      If a modem receives this command, it _should_ leave the network, join
 *      other networks and poll the joined gateway for downlink join confirmed
 *      message with @JOIN_CONFIRMED_PAID is set (when the subscription fee is paid).
 *
 * @GW_FORCE_LEAVE_REASON_SERVER_UNREACHABLE
 *      The gateway cannot reach the backend cloud platform.
 *      \sa beacon_info_t::is_server_connected
 *
 * @GW_FORCE_LEAVE_REASON_JOIN_NEARBY_GW
 *      There may exists some better alternatives for modems to join.
 *      \sa beacon_info_t::nearby_channels
 *
 * @GW_FORCE_LEAVE_INVALID_SHORT_ADDR
 *      The modem's short address can not be found in the joined modem table.
 */
enum _gw_cmd_force_leave_reason {
    GW_FORCE_LEAVE_REASON_OUT_OF_FEE            = 0,
    GW_FORCE_LEAVE_REASON_OUT_OF_FEE_LEAVE      = 1,
    GW_FORCE_LEAVE_REASON_SERVER_UNREACHABLE    = 2,
    GW_FORCE_LEAVE_REASON_JOIN_NEARBY_GW        = 3,
    GW_FORCE_LEAVE_INVALID_SHORT_ADDR           = 4,
    __gw_force_leave_reason_invalid = 16,
};

/**< @} */
/*---------------------------------------------------------------------------*/

#ifdef __GNUC__
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* GW_DOWNLINK_CMD_H_ */
