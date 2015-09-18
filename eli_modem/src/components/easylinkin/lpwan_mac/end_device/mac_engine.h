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

/** radio controller related signals */
#define SIGNAL_RLC_TX_OK                                BV(20)
#define SIGNAL_RLC_TX_TIMEOUT                           BV(21)
#define SIGNAL_RLC_TX_CCA_FAILED                        BV(22)
#define SIGNAL_RLC_TX_CCA_CRC_FAIL                      BV(23)
#define SIGNAL_RLC_RX_DURATION_TIMEOUT                  BV(24)
#define SIGNAL_RLC_RX_OK                                BV(25)

#define SIGNAL_MAC_ENGINE_INIT_FINISHED                 BV(3)

/** join related signals */
#define SIGNAL_MAC_ENGINE_DELAYED_JOINING               BV(4)
#define SIGNAL_MAC_ENGINE_START_JOINING                 BV(5)
#define SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST             BV(6)
#define SIGNAL_MAC_ENGINE_WAIT_JOIN_RESPONSE_TIMEOUT    BV(7)

/** beacon related signals */
#define SIGNAL_MAC_ENGINE_UPDATE_BEACON_INFO            BV(8)   // try to update beacon
#define SIGNAL_MAC_ENGINE_TRACK_BEACON                  BV(9)   // start to track beacon frame
#define SIGNAL_MAC_ENGINE_TRACK_BEACON_TIMEOUT          BV(10)  // track beacon timeout
#define SIGNAL_MAC_ENGINE_BEACON_FOUND                  BV(11)  // beacon found
#define SIGNAL_MAC_ENGINE_LOST_BEACON                   BV(12)  // lost sync with beacon

/** after joined, tx message or command */
#define SIGNAL_MAC_ENGINE_SEND_FRAME                    BV(13)
#define SIGNAL_MAC_ENGINE_SEND_FRAME_CCA                BV(14)
#define SIGNAL_MAC_ENGINE_FRAME_RECEIVED                BV(15)

/**< @} */
/*---------------------------------------------------------------------------*/

/**< The max timer value set by MAC engine. */
#define DEVICE_MAC_ENGINE_MAX_TIMER_DELTA_MS        (((os_uint32)1)<<31)
/**< the longest time to find beacon during joining process */
#define DEVICE_JOINING_FIND_BEACON_TIMEOUT_MS       (1000 * (1 + gl_beacon_period_length_list[LPWAN_BEACON_PERIOD]))

#define DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS       200
#define DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS          500

/**< If the MAC will be idle for at least 12ms, we put MAC into low-power mode. */
#define DEVICE_MAC_SLEEP_NEXT_TIMER_LENGTH_MS       12

#define DEVICE_MAC_MIN_CF_RATIO                     8

enum device_mac_states {
    DE_MAC_STATES_DEFAULT = 0,
    DE_MAC_STATES_INITED,     /**< nothing */
    
    /**< join / rejoin request */
    DE_MAC_STATES_JOINING,
    DE_MAC_STATES_JOINED,   /**< has just joined network */

    // todo
    DE_MAC_STATES_LEAVING,
    DE_MAC_STATES_LEFT,
    DE_MAC_STATES_FORCE_LEFT,
};

enum device_mac_joining_states {
    DE_JOINING_STATES_DELAY_TO_TRACK_BEACON = 0, /**< delay some time to wait beacon */
    DE_JOINING_STATES_TRACKING_BEACON,   /**< Is waiting beacon.
                                             If found beacon, change to state @DE_JOINING_STATES_BEACON_FOUND;
                                             else, change to state @DE_JOINING_STATES_DELAY_TO_WAIT_BEACON. */
    DE_JOINING_STATES_TRACKING_BEACON_TIMEOUT,
    DE_JOINING_STATES_BEACON_FOUND,     /**< Beacon found!
                                             If allow joining, try to join the network;
                                             else, change to state @DE_JOINING_STATES_DELAY_TO_WAIT_BEACON.
                                             todo (the end-device can try to track beacons of different channels) */
    DE_JOINING_STATES_WAIT_JOIN_RESPONSE,
};

enum device_mac_joined_states {
    DE_JOINED_STATES_IDLE = 0,
    DE_JOINED_STATES_TRACKING_BEACON,
    DE_JOINED_STATES_TRACKING_BEACON_TIMEOUT,
    /**
     * Both the tx_frame and rx_frame states should be moved to "radio controller".
     * todo
     */
    DE_JOINED_STATES_TX_FRAME,          /**< has tracked and found beacon, ready
                                         to tx frames */
    DE_JOINED_STATES_RX_FRAME,
};

struct lpwan_device_mac_info {
    /** state machine transitions */
    enum device_mac_states          mac_engine_states;
    enum device_mac_joining_states  joining_states;
    enum device_mac_joined_states   joined_states;

    /** uplink frames' information: pending tx, wait ack */
    struct list_head wait_ack_frame_buffer_list;    /**< wait for ack(s) */
    struct list_head pending_frame_buffer_list;     /**< pending to tx */
    os_uint8 wait_ack_list_len;     /**< \sa LPWAN_DEVICE_MAC_WAIT_ACK_FRAME_MAX_NUM */
    os_uint8 pending_list_len;      /**< \sa LPWAN_DEVICE_MAC_PENDING_TX_FRAME_MAX_NUM */

    /** address information */
    modem_uuid_t uuid;              /**< the unique ID of each modem */
    short_modem_uuid_t suuid;       /**< generate from @uuid */
    short_addr_t short_addr;        /**< when join the network, the modem will have a @short_addr*/
    multicast_addr_t multi_addr;    /**< when join the network, the modem will have a @multi_addr */

    short_addr_t gateway_cluster_addr;

    /** Set if need to check packed ACK. */
    os_boolean is_check_packed_ack; /**< check packed ack in beacon? */

    /** beacon sync information */
    os_boolean is_beacon_synchronized;  /**< if lost beacon, set to OS_FALSE */
    struct time beacon_sync_time;       /**< the time of last beacon synchronization */

    /** The expected beacon sequence ID etc. */
    os_int8 expected_beacon_seq_id;
    os_uint8 expected_beacon_classes_num;  /**< still the same classes number? */
    os_uint8 expected_class_seq_id;

    os_uint8 tx_frame_seq_id;  /**< initialize as a random value when join network,
                                 increment 1 for each new tx frame */

    /**
     * The three beacon-related structures below are synchronized beacon information.
     */
    struct parsed_beacon_info               synced_beacon_info;
    struct parsed_beacon_packed_ack_to_me   synced_beacon_packed_ack_to_me;
};

void device_mac_engine_init(os_uint8 priority);
enum device_mac_states device_get_mac_states(void);

#define DEVICE_SEND_MSG_ERR_INVALID_LEN             -1
#define DEVICE_SEND_MSG_ERR_NOT_JOINED              -2
#define DEVICE_SEND_MSG_ERR_PENDING_TX_BUFFER_FULL  -3
#define DEVICE_SEND_MSG_ERR_FRAME_BUFFER_FULL       -4  // cannot allocate more frame buffer
os_int8 device_mac_send_msg(enum device_message_type type, const os_uint8 msg[], os_uint8 len);

#ifdef __cplusplus
}
#endif

#endif /* MAC_ENGINE_H_ */
