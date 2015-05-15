/**
 * device_uplink_cmd.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef DEVICE_UPLINK_CMD_H_
#define DEVICE_UPLINK_CMD_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "device_uplink_common.h"

#ifdef __GNUC__
#pragma pack(push)
#pragma pack(1)
#endif

__LPWAN struct device_uplink_cmd_common {
    struct device_uplink_common hdr;
    os_uint8 seq;
};

/*---------------------------------------------------------------------------*/
/**< device cmd: active leave @{ */

__LPWAN struct device_cmd_leave {
    /**
     * bits 4: cmd_id   \sa enum cmd_id_end_device
     * bits 4: leave_reason \sa enum _device_leave_reason
     */
    os_uint8 cmd_info;
    token_code_t rejoin_token;
};

enum _device_leave_reason {
    DEVICE_LEAVE_REASON_DEFAULT = 0,
    _device_leave_reason_invalid = 16,
};

/**< @} */
/*---------------------------------------------------------------------------*/
/**< device cmd: reserve info @{ */

/**
 * bits 2: op1_reserve_type \sa enum _device_reserve_type
 * bits 2: op1_demand       \sa enum _device_reserve_demand
 * bits 2: op2_reserve_type
 * bits 2: op2_demand
 */
typedef os_uint8 device_reserve_details_t;

__LPWAN struct device_cmd_reserve_request {
    /**
     * bits 4: cmd_id   \sa enum cmd_id_end_device
     * bits 3: _reserved
     * bits 1: is_check_multicast?
     */
    os_uint8 cmd_info;
    device_reserve_details_t details;
};

/**
 * \brief Reserve OP1/2 slot.
 * \remark There are at most 16 groups. RESERVE_OP12_1x is the fastest one, which
 *         means the modem would like to reserve slot within every beacon group.
 *         RESERVE_OP12_16x is the slowest one, which means the modem would like
 *         to reserve slot every 16 beacon groups.
 * \remark The reservation range should be within:
 *         [0, struct beacon_info_t::beacon_groups_num]
 */
enum _reserve_op12_demand_expand {
    RESERVE_OP12_1x        = 0,
    RESERVE_OP12_2x        = 1,
    RESERVE_OP12_3x        = 2,
    RESERVE_OP12_4x        = 3,
    RESERVE_OP12_5x        = 4,
    RESERVE_OP12_6x        = 5,
    RESERVE_OP12_7x        = 6,
    RESERVE_OP12_8x        = 7,
    RESERVE_OP12_9x        = 8,
    RESERVE_OP12_10x       = 9,
    RESERVE_OP12_11x       = 10,
    RESERVE_OP12_12x       = 11,
    RESERVE_OP12_13x       = 12,
    RESERVE_OP12_14x       = 13,
    RESERVE_OP12_15x       = 14,
    RESERVE_OP12_16x       = 15,
    _reserve_op12_invalid = 16,
};

/**
 * \brief Users only need to choose the fastest (RESERVE_OP12_1x) or the slowest
 *      (around every 65 minutes).
 * \sa enum _beacon_max_groups_num
 */
enum _device_reserve_demand {
    RESERVE_DEMAND_FASTEST = 0,   /**< RESERVE_OP12_1x, around 4min ~ 10min.
                                     \sa enum _beacon_period */
    RESERVE_DEMAND_FAST    = 1,   /**< around every 25 minutes */
    RESERVE_DEMAND_SLOW    = 2,   /**< around every 45 minutes */
    RESERVE_DEMAND_SLOWEST = 3,   /**< \sa enum _beacon_max_groups_num */
    _reserve_demand_speed_invalid = 4,
};

enum _device_reserve_type {
    DEVICE_RESERVE_DO_NOTHING = 0,
    DEVICE_PERFORM_RESERVE,
    DEVICE_CANCEL_RESERVE,
    _de_reserve_type_invalid = 4,
};

/**< @} */
/*---------------------------------------------------------------------------*/

#ifdef __GNUC__
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* DEVICE_UPLINK_CMD_H_ */
