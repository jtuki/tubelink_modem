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

__LPWAN struct gw_downlink_cmd_common {
    gw_downlink_common_hdr_t hdr;
    os_uint8 seq;
};

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
 *      command, it is not allowed to send any emergent/event message; and the
 *      modem _should_ poll the gateway for downlink join confirmed message with
 *      @JOIN_CONFIRMED_PAID is set (when the subscription fee is paid).
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
