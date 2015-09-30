/**
 * mac_engine.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef MAC_ENGINE_H_
#define MAC_ENGINE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lpwan_types.h"
#include "kernel/process.h"
#include "kernel/timer.h"

#include "frame_defs/common_defs.h"

#include "protocol_utils/parse_frame_hdr.h"
#include "protocol_utils/parse_beacon.h"

#include "lpwan_utils.h"
#include "lpwan_radio_config_sets.h"

extern os_pid_t gl_device_mac_engine_pid;

extern struct parsed_beacon_info *_s_info;

/*---------------------------------------------------------------------------*/
/**< signals @{ */

#define SIGNAL_MAC_ENGINE_INIT_FINISHED                 BV(1)
#define SIGNAL_MAC_ENGINE_START_JOINING                 BV(2)

/** join related signals */
#define SIGNAL_MAC_ENGINE_SEARCH_BEACON                 BV(5)

/** beacon related signals */
#define SIGNAL_MAC_ENGINE_BEACON_FOUND                  BV(10)  // beacon found
#define SIGNAL_MAC_ENGINE_BEACON_NOT_FOUND              BV(11)
#define SIGNAL_MAC_ENGINE_BEACON_TRACKED                BV(12)
#define SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON      BV(13)  // lost beacon, get ready to track again
#define SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED           BV(14)  // beacon tracking failed

/** after joined, tx message or command */
#define SIGNAL_MAC_ENGINE_SEND_FRAME                    BV(18)
#define SIGNAL_MAC_ENGINE_RECV_FRAME                    BV(19)
#define SIGNAL_MAC_ENGINE_RECV_TIMEOUT                  BV(20)

#include "radio_controller/rlc_callback_signals.h"      // range from BV(25) ~ BV(30)

/**< @} */
/*---------------------------------------------------------------------------*/

/**< the longest time to find beacon during joining process */
#define DEVICE_JOINING_SEARCH_BCN_TIMEOUT_MS        (1000 * (1 + gl_beacon_period_length_list[LPWAN_BEACON_PERIOD]))

#define DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS       100
#define DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS          300

#define DEVICE_MAC_RECV_DOWNLINK_IN_ADVANCE_MS      50
#define DEVICE_MAC_RECV_DOWNLINK_TIMEOUT_MS         300

/** BEACON_PERIOD_SLOTS_NUM - (1+10)*LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS;
 * 1 for beacon, 10 for possible downlink frames.
 *
 * \sa LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS
 * \sa BEACON_PERIOD_SLOTS_NUM
 */
#define DE_MAC_ENGINE_MIN_UPLINK_SLOTS              60

enum device_mac_states {
    DE_MAC_STATES_DEFAULT = 0,
    DE_MAC_STATES_INITED,     /**< nothing */
    
    /**< join / rejoin request */
    DE_MAC_STATES_JOINING,
    DE_MAC_STATES_JOINED,   /**< has just joined network */
};

enum device_mac_joining_states {
    DE_JOINING_STATES_SEARCH_BEACON,
    DE_JOINING_STATES_BEACON_FOUND,
    DE_JOINING_STATES_TX_JOIN_REQ,      /**< prepare and tx JOIN_REQ */
    DE_JOINING_STATES_RX_JOIN_CONFIRM,  /**< get ready to rx JOIN_CONFIRM */
};

enum device_mac_joined_states {
    DE_JOINED_STATES_IDLE = 0,
    DE_JOINED_STATES_TX_FRAME,
    DE_JOINED_STATES_RX_FRAME,
};

struct lpwan_device_mac_info {
    /** state machine transitions */
    enum device_mac_states          mac_engine_states;
    enum device_mac_joining_states  joining_states;
    enum device_mac_joined_states   joined_states;

    /** address information */
    modem_uuid_t uuid;              /**< the unique ID of each modem */
    short_modem_uuid_t suuid;       /**< generate from @uuid */
    short_addr_t short_addr;        /**< available after joined the network */

    app_id_t app_id;

    short_addr_t gateway_cluster_addr;
};

void device_mac_engine_init(os_uint8 priority);
enum device_mac_states mac_info_get_mac_states(void);

void mac_info_get_uuid(modem_uuid_t *uuid);
short_modem_uuid_t mac_info_get_suuid(void);
short_addr_t mac_info_get_short_addr(void);
app_id_t mac_info_get_app_id(void);

os_boolean mac_engine_is_allow_tx(os_uint8 class_seq_id);

#define DEVICE_SEND_MSG_ERR_INVALID_LEN             -1
#define DEVICE_SEND_MSG_ERR_NOT_JOINED              -2
#define DEVICE_SEND_MSG_ERR_FRAME_BUFFER_FULL       -3  // cannot allocate more frame buffer
os_int8 device_mac_send_msg(enum device_message_type type, const os_uint8 msg[], os_uint8 len);

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_H_ */
