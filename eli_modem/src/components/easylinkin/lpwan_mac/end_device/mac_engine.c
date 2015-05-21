/**
 * mac_engine.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "kernel/process.h"
#include "kernel/timer.h"
#include "kernel/ipc.h"
#include "kernel/sys_signal_defs.h"
#include "lib/assert.h"
#include "lib/mem_pool.h"
#include "lib/linked_list.h"
#include "lib/hdk_utilities.h"

#include "frame_defs/frame_defs.h"

#include "lpwan_config.h"
#include "end_device/config_end_device.h"
#include "mac_engine_config.h"

#include "hal/hal_mcu.h"
#include "radio/lpwan_radio.h"

#include "protocol_utils/parse_frame_hdr.h"
#include "protocol_utils/parse_beacon.h"
#include "protocol_utils/construct_frame_hdr.h"
#include "protocol_utils/construct_de_uplink_msg.h"

#include "mac_engine.h"

/*---------------------------------------------------------------------------*/
/**< MAC information base etc. @{ */

struct tx_frame_buffer {
    struct list_head hdr;

    /**
     * If @transmit_times == 0 (has not yet been transmitted), the three
     * variables below are not used.
     */
    os_uint8 transmit_times;        /**< if 0, has not yet been sent. */
    os_uint8 channel_backoff_num;   /**< todo */
    os_uint8 seq;
    os_int8 beacon_seq_id;          /**< the sequence id when the message is sent. */
    os_int8 expected_beacon_seq_id; /**< the expected seq id to received ACK. */

    enum frame_type_end_device ftype;
    enum device_message_type msg_type;  /**< if @ftype is FTYPE_DEVICE_MSG, use
                                           this @msg_type to decide its type. */
    os_uint8 len;
    os_uint8 frame_buffer[LPWAN_DEVICE_MAC_UPLINK_MTU];
};

/**
 * \sa LPWAN_DEVICE_MAC_UPLINK_BUFFER_MAX_NUM
 * \sa sizeof(struct tx_frame_buffer)
 */
static struct mem_pool_hdr *_uplink_frame_pool;

static struct lpwan_device_mac_info mac_info;

/**< @} */
/*---------------------------------------------------------------------------*/
/**< static function declarations @{ */

static signal_bv_t device_mac_engine_entry(os_pid_t pid, signal_bv_t signal);
static inline void mac_state_transfer(enum device_mac_states state);
static inline void mac_joining_state_transfer(enum device_mac_joining_states state);
static inline void mac_joined_state_transfer(enum device_mac_joined_states state);

static os_int8 rx_and_see_is_it_a_beacon(os_uint8 *rx_buf, os_uint8 len,
                    struct parsed_frame_hdr_info *frame_hdr_info,
                    struct parsed_beacon_info *beacon_info,
                    struct parsed_beacon_packed_ack_to_me *beacon_packed_ack,
                    struct parsed_beacon_op2_to_me *beacon_op2);

static void update_synced_beacon_info(struct parsed_frame_hdr_info *f_hdr,
                                      struct parsed_beacon_info *info,
                                      struct parsed_beacon_packed_ack_to_me *ack,
                                      struct parsed_beacon_op2_to_me *op2);

static inline os_int8 get_expected_beacon_seq_id(os_int8 cur_seq_id,
                                                 os_int8 packed_ack_delay_num);

static void update_device_tx_frame_msg(struct device_uplink_msg *msg,
                                       const struct tx_frame_buffer *buffer,
                                       os_uint8 beacon_group_seq_id,
                                       os_uint8 beacon_class_seq_id);

static void device_mac_get_uuid(modem_uuid_t *uuid);
static void device_mac_calculate_suuid(void);
static void device_mac_srand(void);

/**< When the MAC wake up, check the radio periodically. */
static struct timer *radio_check_timer = NULL;

static void mac_engine_wakeup_init_callback(void);
static os_int8 mac_engine_sleep_cleanup_callback(void);
/**< @} */
/*---------------------------------------------------------------------------*/
/**< file-scope resources @{ */

/**
 * the destination and source of a frame.
 */
static struct lpwan_addr _frame_dest;
static struct lpwan_addr _frame_src;

/**< @} */
/*---------------------------------------------------------------------------*/
/**< debug related variables @{ */
#define xLPWAN_MAC_DEBUG_ENABLE_MARK_RUNNING_LINE
#define LPWAN_MAC_DEBUG_ENABLE_MARK_RUNNING_LINE_BLOCK
#define LPWAN_MAC_DEBUG_ENABLE_MARK_LINE_TIMER_START

/**
 * Mark the current running line.
 * To diagnose which line is running before the program run out ot control.
 *
 * \sa _lpwan_debug_mark_running_line_block()
 */
#ifdef LPWAN_MAC_DEBUG_ENABLE_MARK_RUNNING_LINE
static os_size_t debug_running_line__; // the __LINE__ running
#define _lpwan_debug_mark_running_line() do {   \
    debug_running_line__ = __LINE__;            \
    if (debug_running_line__ == -1)             \
        while (1) {}                            \
} while (0)
#else
#define _lpwan_debug_mark_running_line()
#endif

/**
 * Mark the current running line __within__ some block.
 * To diagnose which line is running before the program run out ot control.
 *
 * \sa _lpwan_debug_mark_running_line()
 */
#ifdef LPWAN_MAC_DEBUG_ENABLE_MARK_RUNNING_LINE_BLOCK
static os_size_t _debug_running_line_sub_block; // the __LINE__ running within a block
#define _lpwan_debug_mark_running_line_block() do { \
    _debug_running_line_sub_block = __LINE__;       \
    if (_debug_running_line_sub_block == -1)        \
        while (1) {}                                \
} while (0)
#else
#define _lpwan_debug_mark_running_line_block()
#endif

#ifdef LPWAN_MAC_DEBUG_ENABLE_MARK_LINE_TIMER_START
static os_size_t _debug_mark_line_timer_start;
#define _lpwan_debug_mark_ling_timer_start() do {   \
    _debug_mark_line_timer_start = __LINE__;        \
    if (_debug_mark_line_timer_start == -1)         \
        while (1) {}                                \
} while (0)
#else
#define _lpwan_debug_mark_ling_timer_start()
#endif

/**< @} */
/*---------------------------------------------------------------------------*/

os_pid_t gl_device_mac_engine_pid;
haddock_process("device_mac_engine");

/**
 * the MAC's rx buffer for radio's rx frames
 */
static os_uint8 device_mac_rx_buffer[1+LPWAN_RADIO_RX_BUFFER_MAX_LEN];

void device_mac_engine_init(void)
{
    struct process *proc_mac_engine = process_create(device_mac_engine_entry,
                                                     PRIORITY_DEVICE_MAC_ENGINE);
    haddock_assert(proc_mac_engine);

    gl_device_mac_engine_pid = proc_mac_engine->_pid;
#if defined HDK_CFG_POWER_SAVING_ENABLE && HDK_CFG_POWER_SAVING_ENABLE == OS_TRUE
    proc_mac_engine->wakeup_init_callback   = mac_engine_wakeup_init_callback;
    proc_mac_engine->sleep_cleanup_callback = mac_engine_sleep_cleanup_callback;
#else
    (void) mac_engine_wakeup_init_callback;
    (void) mac_engine_sleep_cleanup_callback;
#endif
    
    /** initialize the @mac_info @{ */
    haddock_memset(& mac_info, 0, sizeof(mac_info));
    
    _uplink_frame_pool = \
            mem_pool_create(LPWAN_DEVICE_MAC_UPLINK_BUFFER_MAX_NUM,
                            sizeof(struct tx_frame_buffer));
    haddock_assert(_uplink_frame_pool);

    list_head_init(& mac_info.wait_ack_frame_buffer_list);
    list_head_init(& mac_info.pending_frame_buffer_list);
    mac_info.wait_ack_list_len = 0;
    mac_info.pending_list_len = 0;

    device_mac_get_uuid(& mac_info.uuid);
    device_mac_calculate_suuid();
    /** @} */

    device_mac_srand();

    mac_state_transfer(DE_MAC_STATES_INITED);
    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_INIT_FINISHED);

    lpwan_radio_register_mac_pid(gl_device_mac_engine_pid);
    /** initialize the radio interface */
    lpwan_radio_init();
}

static struct tx_frame_buffer *_alloc_tx_frame_buffer(void)
{
    print_debug_str("sn: _alloc_tx_frame_buffer @{");
    struct mem_pool_blk *blk = \
            mem_pool_alloc_blk(_uplink_frame_pool, sizeof(struct tx_frame_buffer));
    print_debug_str("sn: @}");
    return (blk == NULL) ? NULL : ((struct tx_frame_buffer*) blk->blk);
}

static void _free_tx_frame_buffer(struct tx_frame_buffer *buffer)
{
    print_debug_str("sn: free frame buffer @{");
    struct mem_pool_blk *blk = find_mem_pool_blk(buffer);
    mem_pool_free_blk(blk);
    print_debug_str("sn: @}");
}

/**
 * Send message (emergent/event/routine) after the end-device(s) have joined
 * the network.
 *
 * @return 0 if ok, negative value if failed.
 * \sa DEVICE_SEND_MSG_ERR_xxx
 */
os_int8 device_mac_send_msg(enum device_message_type type, const os_uint8 msg[], os_uint8 len)
{
    if (mac_info.mac_engine_states != DE_MAC_STATES_JOINED)
        return DEVICE_SEND_MSG_ERR_NOT_JOINED;

    if (len > LPWAN_MAX_PAYLAOD_LEN)
        return DEVICE_SEND_MSG_ERR_INVALID_LEN;

    if (mac_info.pending_list_len >= LPWAN_DEVICE_MAC_PENDING_TX_FRAME_MAX_NUM) {
        return DEVICE_SEND_MSG_ERR_PENDING_TX_BUFFER_FULL;
    }

    struct tx_frame_buffer *frame_buffer = _alloc_tx_frame_buffer();
    if (! frame_buffer) {
        return DEVICE_SEND_MSG_ERR_FRAME_BUFFER_FULL;
    }

    frame_buffer->ftype = FTYPE_DEVICE_MSG;
    frame_buffer->msg_type = type;
    frame_buffer->transmit_times = 0;

    // construct the frame header
    _frame_dest.type = ADDR_TYPE_SHORT_ADDRESS;
    _frame_dest.addr.short_addr = mac_info.gateway_cluster_addr;
    _frame_src.type = ADDR_TYPE_SHORT_ADDRESS;
    _frame_src.addr.short_addr = mac_info.short_addr;

    construct_device_frame_header(frame_buffer->frame_buffer, LPWAN_DEVICE_MAC_UPLINK_MTU,
                                  FTYPE_DEVICE_MSG,
                                  & _frame_src, & _frame_dest,
                                  OS_FALSE, // is_mobile?
                                  RADIO_TX_POWER_LEVELS_NUM-1);

    // construct the uplink message
    construct_device_uplink_msg(type, msg, len,
                                & frame_buffer->frame_buffer[FRAME_HDR_LEN_NORMAL],
                                LPWAN_DEVICE_MAC_UPLINK_MTU - FRAME_HDR_LEN_NORMAL);
    frame_buffer->len = FRAME_HDR_LEN_NORMAL + sizeof(struct device_uplink_msg) + len;

    list_add_tail(& frame_buffer->hdr, & mac_info.pending_frame_buffer_list);
    mac_info.pending_list_len += 1;

    return 0;
}

enum device_mac_states device_get_mac_states(void)
{
    return mac_info.mac_engine_states;
}

#define timer_not_started(timer)  (list_empty(& (timer)->hdr))

static signal_bv_t device_mac_engine_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);
    
    static struct parsed_frame_hdr_info             _frame_hdr_info;
    static struct parsed_beacon_info                _beacon_info;
    static struct parsed_beacon_packed_ack_to_me    _beacon_packed_ack;
    static struct parsed_beacon_op2_to_me           _beacon_op2;

    static os_uint8 *rx_buf = device_mac_rx_buffer;

    static struct timer *_timeout_timer = NULL; /**< internal timeout timer
                                                  \sa joining_timer
                                                  \sa tx_timer
                                                  */
    static struct timer *update_beacon_timer = NULL; // try to track beacon periodically
    static struct timer *wait_beacon_timeout_timer = NULL; // wait beacon timeout

    /**
     * \remark The two timers below use @_timeout_timer.
     * Only one timer _should_ exist at any time.
     *
     * \sa _timeout_timer
     */
    static struct timer *joining_timer = NULL;
    static struct timer *tx_timer = NULL;

    static os_boolean _is_lost_beacon = OS_FALSE;

    static os_boolean _run_first_time = OS_TRUE;
    if (_run_first_time) {
        _timeout_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                        DEVICE_MAC_ENGINE_MAX_TIMER_DELTA_MS);
        haddock_assert(_timeout_timer);

        update_beacon_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                        DEVICE_MAC_ENGINE_MAX_TIMER_DELTA_MS);
        haddock_assert(update_beacon_timer);

        wait_beacon_timeout_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                        DEVICE_MAC_ENGINE_MAX_TIMER_DELTA_MS);
        haddock_assert(wait_beacon_timeout_timer);

        radio_check_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                        DEVICE_MAC_ENGINE_MAX_TIMER_DELTA_MS);
        haddock_assert(radio_check_timer);

        _run_first_time = OS_FALSE;
    }

    static os_uint8 _channel_backoff_num;

    static struct parsed_beacon_info *_s_info = \
            & mac_info.synced_beacon_info;
    static struct parsed_beacon_packed_ack_to_me *_s_ack = \
            & mac_info.synced_beacon_packed_ack_to_me;
    static struct parsed_beacon_op2_to_me *_s_op2 = \
            & mac_info.synced_beacon_op2_to_me;

    // we don't use it currently
    (void) _s_op2;

    if (signal & SIGNAL_MAC_ENGINE_UPDATE_BEACON) {
        /** \sa update_beacon_timer */

        haddock_assert(mac_info.is_beacon_synchronized);
        _lpwan_debug_mark_running_line();

        mac_info.expected_beacon_seq_id += 1;
        if (mac_info.expected_beacon_seq_id == BEACON_MAX_SEQ_NUM) {
            mac_info.expected_beacon_seq_id = 0;
        }

        mac_info.expected_class_seq_id += 1;
        if (mac_info.expected_class_seq_id > _s_info->beacon_classes_num) {
            mac_info.expected_class_seq_id = 1;
        }

        mac_info.is_check_packed_ack = OS_FALSE;
        mac_info.is_check_op2 = OS_FALSE;

        os_boolean need_wait_beacon = OS_FALSE;

        if (mac_info.wait_ack_list_len > 0) {
            static struct list_head *pos;
            static struct tx_frame_buffer *buf;

            _lpwan_debug_mark_running_line_block();
            list_for_each(pos, & mac_info.wait_ack_frame_buffer_list) {
                buf = list_entry(pos, struct tx_frame_buffer, hdr);
                if (buf->expected_beacon_seq_id == mac_info.expected_beacon_seq_id) {
                    mac_info.is_check_packed_ack = OS_TRUE;
                    need_wait_beacon = OS_TRUE;
                    break;
                }
            }
        } else if (mac_info.pending_list_len > 0) {
            _lpwan_debug_mark_running_line_block();
            if (((os_uint16) mac_info.short_addr) % mac_info.expected_class_seq_id == 0) {
                need_wait_beacon = OS_TRUE;
            }
        }

        if (need_wait_beacon) {
            /*
             * set signal:  SIGNAL_MAC_ENGINE_WAIT_BEACON
             * set timeout: SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT
             */
            _lpwan_debug_mark_running_line_block();
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_WAIT_BEACON);

            _lpwan_debug_mark_ling_timer_start();
            os_timer_reconfig(wait_beacon_timeout_timer, this->_pid,
                              SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT,
                              DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS);
            os_timer_start(wait_beacon_timeout_timer);
        }

        _lpwan_debug_mark_running_line_block();
        /**
         * restart the beacon tracking timer @{
         */
        os_uint32 _delta = 1000 * _s_info->beacon_period_length;
        _lpwan_debug_mark_running_line_block();
        _lpwan_debug_mark_ling_timer_start();
        os_timer_reconfig(update_beacon_timer, this->_pid,
                          SIGNAL_MAC_ENGINE_UPDATE_BEACON, _delta);
        _lpwan_debug_mark_running_line_block();
        os_timer_start(update_beacon_timer);
        /** @} */

        _lpwan_debug_mark_running_line_block();
        return signal ^ SIGNAL_MAC_ENGINE_UPDATE_BEACON;
    }

    if (signal & SIGNAL_MAC_ENGINE_CHECK_RADIO_TIMEOUT) {
        lpwan_radio_routine();
        _lpwan_debug_mark_running_line();

        _lpwan_debug_mark_ling_timer_start();
        os_timer_reconfig(radio_check_timer, this->_pid,
                          SIGNAL_MAC_ENGINE_CHECK_RADIO_TIMEOUT,
                          DEVICE_MAC_RADIO_PERIODICAL_CHECK_INTERVAL);
        os_timer_start(radio_check_timer);
        return signal ^ SIGNAL_MAC_ENGINE_CHECK_RADIO_TIMEOUT;
    }

    switch ((int) mac_info.mac_engine_states) {
    case DE_MAC_STATES_DEFAULT:
        // don't care the @signal, discard.
        return 0;
    case DE_MAC_STATES_INITED:
        if (signal & SIGNAL_MAC_ENGINE_INIT_FINISHED) {
            // has just finished initialization
            _lpwan_debug_mark_running_line();

            _lpwan_debug_mark_ling_timer_start();
            haddock_assert(timer_not_started(_timeout_timer));
            joining_timer = _timeout_timer;
            os_timer_reconfig(joining_timer,
                              this->_pid, SIGNAL_MAC_ENGINE_START_JOINING,
                              hdk_randr(0, 1000) +
                              1000 * hdk_randr(LPWAN_MAC_JOIN_AFTER_INITED_MIN,
                                               LPWAN_MAC_JOIN_AFTER_INITED_MAX));
            os_timer_start(joining_timer);
            print_debug_str("sn init finish: wait to start joining");

            process_sleep();
            return signal ^ SIGNAL_MAC_ENGINE_INIT_FINISHED;
        }
        else if (signal & SIGNAL_MAC_ENGINE_START_JOINING) {
            // start joining process
            mac_state_transfer(DE_MAC_STATES_JOINING);
            mac_joining_state_transfer(DE_JOINING_STATES_WAIT_BEACON);
            _lpwan_debug_mark_running_line();

            print_debug_str("sn joining: start to wait beacon");
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_WAIT_BEACON);

            /** \ref DEVICE_JOIN_FIND_BEACON_TIMEOUT_MS */
            _lpwan_debug_mark_ling_timer_start();
            os_timer_reconfig(wait_beacon_timeout_timer, this->_pid,
                              SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT,
                              DEVICE_JOIN_FIND_BEACON_TIMEOUT_MS);
            os_timer_start(wait_beacon_timeout_timer);

            return signal ^ SIGNAL_MAC_ENGINE_START_JOINING;
        }
        else {
            return 0;
        }
    case DE_MAC_STATES_JOINING:
        /*
         * signals:
         *      SIGNAL_MAC_ENGINE_WAIT_BEACON
         *      SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT
         *      SIGNAL_MAC_ENGINE_BEACON_FOUND
         *
         *      SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST (todo)
         *      SIGNAL_MAC_ENGINE_WAIT_JOIN_RESPONSE_TIMEOUT (todo)
         *
         *      SIGNAL_LPWAN_RADIO_RX_OK
         *      SIGNAL_LPWAN_RADIO_RX_TIMEOUT
         */
        if (signal & SIGNAL_MAC_ENGINE_WAIT_BEACON) {
            _lpwan_debug_mark_running_line();

            lpwan_radio_stop_rx();
            lpwan_radio_start_rx();

            return signal ^ SIGNAL_MAC_ENGINE_WAIT_BEACON;
        }
        else if (signal & SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT) {
            _lpwan_debug_mark_running_line();

            lpwan_radio_stop_rx();

            print_debug_str("sn joining: wait beacon timeout");
            // restart the joining process #mark#
            mac_state_transfer(DE_MAC_STATES_INITED);
            mac_joining_state_transfer(DE_JOINING_STATES_WAIT_BEACON_TIMEOUT);

            haddock_assert(timer_not_started(wait_beacon_timeout_timer)); // todo
            os_timer_stop(wait_beacon_timeout_timer);
            os_timer_stop(update_beacon_timer);
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_INIT_FINISHED);
            return signal ^ SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT;
        }
        else if (signal & SIGNAL_MAC_ENGINE_BEACON_FOUND) {
            _lpwan_debug_mark_running_line();

            print_debug_str("sn joining: beacon found!");
            /*
             * 1. if join allowed, store the beacon's information to mac_info,
             *    and initiate join request.
             * 2. else, try again later. todo
             */
            if (_beacon_info.is_join_allowed || _is_lost_beacon) {
                update_synced_beacon_info(& _frame_hdr_info,
                                          & _beacon_info,
                                          & _beacon_packed_ack,
                                          & _beacon_op2);

                // set up the periodically timer to track beacon
                os_uint32 _delta = 1000 * _beacon_info.beacon_period_length -
                                   DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS;
                _lpwan_debug_mark_ling_timer_start();
                os_timer_reconfig(update_beacon_timer, this->_pid,
                                  SIGNAL_MAC_ENGINE_UPDATE_BEACON, _delta);
                os_timer_start(update_beacon_timer);

                if (_is_lost_beacon) {
                    /*
                     * todo we simply regard that the end-device has found the beacon
                     * again ...
                     */
                    _is_lost_beacon = OS_FALSE;
                    mac_state_transfer(DE_MAC_STATES_JOINED);
                    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);

                    process_sleep();
                } else {
                    // todo try to send join request to join a network
                    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST);
                }
            } else {
                // we simply try again later #mark# todo
                lpwan_radio_stop_rx();

                mac_state_transfer(DE_MAC_STATES_INITED);
                
                os_timer_stop(wait_beacon_timeout_timer);
                os_timer_stop(update_beacon_timer);
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_INIT_FINISHED);
            }
            return signal ^ SIGNAL_MAC_ENGINE_BEACON_FOUND;
        }
        else if (signal & SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST) {
            _lpwan_debug_mark_running_line();
            /*
             * we simply generate short_addr from modem_uuid currently. #mark#
             */
            mac_info.short_addr = (((os_uint16) mac_info.uuid.addr[7]) << 8)
                                  + mac_info.uuid.addr[6];
            mac_info.tx_frame_seq_id = (os_uint8) hdk_randr(0, 128);
            mac_state_transfer(DE_MAC_STATES_JOINED);
            mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
            
            print_debug_str("sn joined!");

            process_sleep();
            return signal ^ SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST;
        }
        else if (signal & SIGNAL_MAC_ENGINE_WAIT_JOIN_RESPONSE_TIMEOUT) {
            // todo
            return signal ^ SIGNAL_MAC_ENGINE_WAIT_JOIN_RESPONSE_TIMEOUT;
        }
        else if (signal & SIGNAL_LPWAN_RADIO_RX_OK) {
            _lpwan_debug_mark_running_line();
            os_int8 _len = rx_and_see_is_it_a_beacon(rx_buf,
                                        1+LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                                        & _frame_hdr_info,
                                        & _beacon_info,
                                        & _beacon_packed_ack,
                                        & _beacon_op2);
            if (_len > 0) {
                // valid beacon frame from gateway.
                os_timer_stop(wait_beacon_timeout_timer);
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_BEACON_FOUND);
            } else {
                lpwan_radio_start_rx();
            }
            return signal ^ SIGNAL_LPWAN_RADIO_RX_OK;
        }
        else if (signal & SIGNAL_LPWAN_RADIO_RX_TIMEOUT) {
            _lpwan_debug_mark_running_line();
            // before @SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT, restart the rx process.
            lpwan_radio_start_rx();
            return signal ^ SIGNAL_LPWAN_RADIO_RX_TIMEOUT;
        }
        else {
            return 0;
        }
    case DE_MAC_STATES_JOINED:
        /*
         * signals:
         *      SIGNAL_MAC_ENGINE_WAIT_BEACON
         *      SIGNAL_MAC_ENGINE_BEACON_FOUND
         *      SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT
         */
        switch ((int) mac_info.joined_states) {
        case DE_JOINED_STATES_IDLE:
            if (signal & SIGNAL_MAC_ENGINE_WAIT_BEACON) {
                _lpwan_debug_mark_running_line();

                mac_joined_state_transfer(DE_JOINED_STATES_WAIT_BEACON);
                print_debug_str("sn joined: start to track beacon!");

                lpwan_radio_stop_rx();
                lpwan_radio_start_rx();

                return signal ^ SIGNAL_MAC_ENGINE_WAIT_BEACON;
            }
            else {
                _lpwan_debug_mark_running_line();
                return 0;
            }
        case DE_JOINED_STATES_WAIT_BEACON:
            if (signal & SIGNAL_LPWAN_RADIO_RX_TIMEOUT) {
                _lpwan_debug_mark_running_line();
                lpwan_radio_start_rx();
                return signal ^ SIGNAL_LPWAN_RADIO_RX_TIMEOUT;
            }
            else if (signal & SIGNAL_LPWAN_RADIO_RX_OK) {
                _lpwan_debug_mark_running_line();
                os_int8 _len = rx_and_see_is_it_a_beacon(rx_buf,
                                            1+LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                                            & _frame_hdr_info,
                                            & _beacon_info,
                                            & _beacon_packed_ack,
                                            & _beacon_op2);
                if (_len > 0) {
                    // valid beacon frame from gateway.
                    os_timer_stop(wait_beacon_timeout_timer);
                    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_BEACON_FOUND);
                    print_debug_str("sn joined: beacon found.");
                }  else {
                    lpwan_radio_start_rx();
                }
                return signal ^ SIGNAL_LPWAN_RADIO_RX_OK;
            }
            else if (signal & SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT) {
                _lpwan_debug_mark_running_line();
                /*
                 * lost beacon of gateway, try to find beacon again.
                 */
                mac_joined_state_transfer(DE_JOINED_STATES_WAIT_BEACON_TIMEOUT);

                struct list_head *pos;
                struct list_head *n;
                /*
                 * Move all the frames from @wait_ack_frame_buffer_list to
                 * @pending_frame_buffer_list.
                 */
                list_for_each_safe(pos, n, & mac_info.wait_ack_frame_buffer_list) {
                    list_move_tail(pos, & mac_info.pending_frame_buffer_list);
                }
                mac_info.pending_list_len += mac_info.wait_ack_list_len;
                mac_info.wait_ack_list_len = 0;

                _is_lost_beacon = OS_TRUE;
                os_timer_stop(update_beacon_timer);
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_LOST_BEACON);

                return signal ^ SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT;
            }
            else if (signal & SIGNAL_MAC_ENGINE_BEACON_FOUND) {
                _lpwan_debug_mark_running_line();
                update_synced_beacon_info(& _frame_hdr_info,
                                          & _beacon_info,
                                          & _beacon_packed_ack,
                                          & _beacon_op2);

                if (_s_info->beacon_seq_id == mac_info.expected_beacon_seq_id
                    && _s_info->beacon_classes_num == mac_info.expected_beacon_classes_num
                    && _s_info->beacon_class_seq_id == mac_info.expected_class_seq_id)
                {
                    static struct list_head *pos;
                    static struct list_head *n;
                    static struct tx_frame_buffer *buffer;

                    print_debug_str("sn joined: beacon id match");

                    // check packed ack
                    if (_s_info->has_packed_ack && _s_ack->has_ack) {
                        _lpwan_debug_mark_running_line();
                        list_for_each_safe(pos, n, & mac_info.wait_ack_frame_buffer_list) {
                            buffer = list_entry(pos, struct tx_frame_buffer, hdr);
                            if (buffer->expected_beacon_seq_id == _s_info->beacon_seq_id) {
                                /*
                                 *  Has received ack!
                                 *
                                 *  We _should_ signal the upper layer that
                                 *  "I have received ack with confirm_seq @seq"
                                 *  todo
                                 */
                                print_debug_str("sn: get ACK!");

                                list_del_init(pos);
                                _free_tx_frame_buffer(buffer);
                                mac_info.wait_ack_list_len -= 1;
                            } else if (buffer->expected_beacon_seq_id < _s_info->beacon_seq_id) {
                                // Miss the ack! todo
                                print_debug_str("sn: no ACK for me!");

                                if (buffer->transmit_times < LPWAN_MAX_RETRANSMIT_NUM) {
                                    list_move_tail(pos, & mac_info.pending_frame_buffer_list);
                                    mac_info.wait_ack_list_len -= 1;
                                    mac_info.pending_list_len += 1;
                                } else {
                                    // too many re-transmissions, discard the packets.
                                    list_del_init(pos);
                                    _free_tx_frame_buffer(buffer);
                                    mac_info.wait_ack_list_len -= 1;
                                }
                            }
                        }
                    } else if (mac_info.is_check_packed_ack == OS_TRUE) {
                        _lpwan_debug_mark_running_line();
                        // we expect to receive packed ack, but not.
                        print_debug_str("sn: no ACK!");
                        
                        list_for_each_safe(pos, n, & mac_info.wait_ack_frame_buffer_list) {
                            buffer = list_entry(pos, struct tx_frame_buffer, hdr);
                            if (buffer->expected_beacon_seq_id <= mac_info.expected_beacon_seq_id) {
                                if (buffer->transmit_times < LPWAN_MAX_RETRANSMIT_NUM) {
                                    list_move_tail(pos, & mac_info.pending_frame_buffer_list);
                                    mac_info.wait_ack_list_len -= 1;
                                    mac_info.pending_list_len += 1;
                                } else {
                                    // too many re-transmissions, discard the packets.
                                    list_del_init(pos);
                                    _free_tx_frame_buffer(buffer);
                                    mac_info.wait_ack_list_len -= 1;
                                }
                            }
                        }
                    }

                    // check op1 and op2 todo
                    if (_s_info->has_op2) {}
                    
                    /*
                     * Check tx.
                     * We use all the emergent and event and routine right now.
                     * todo
                     */
                    if ((! list_empty(& mac_info.pending_frame_buffer_list))
                        && mac_info.allow_tx_info.allow_msg_emergent >= 0)
                    {
                        static os_uint32 _delay;
                        _delay = (_s_info->beacon_section_length_us *
                                 mac_info.allow_tx_info.allow_msg_emergent)
                                 / 1000;
                        _lpwan_debug_mark_running_line();

                        if (_delay > 0) {
                            haddock_assert(timer_not_started(_timeout_timer));
                            tx_timer = _timeout_timer;
                            _lpwan_debug_mark_ling_timer_start();
                            os_timer_reconfig(tx_timer, this->_pid,
                                              SIGNAL_MAC_ENGINE_SEND_FRAME, _delay);
                            os_timer_start(tx_timer);
                        } else {
                            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_SEND_FRAME);
                        }

                        mac_joined_state_transfer(DE_JOINED_STATES_TX_FRAME);

                        if (_delay >= DEVICE_MAC_SLEEP_NEXT_TIMER_LENGTH_MS) {
                            process_sleep();
                        }
                    } else {
                        mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                    }
                }
                else {
                    _lpwan_debug_mark_running_line();
                    // we regard it as lost beacon todo
                    print_debug_str("sn joined: beacon id mis-match");
                    os_timer_stop(update_beacon_timer);
                    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_WAIT_BEACON_TIMEOUT);
                }
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_FOUND;
            }
            else {
                return 0;
            }
        case DE_JOINED_STATES_WAIT_BEACON_TIMEOUT:
            if (signal & SIGNAL_MAC_ENGINE_LOST_BEACON) {
                _lpwan_debug_mark_running_line();
                haddock_assert(_is_lost_beacon);
                print_debug_str("sn joined: lost beacon ...");

                lpwan_radio_stop_rx();
                lpwan_radio_start_rx();

                // stop updating the beacon timer.
                os_timer_stop(update_beacon_timer);
                os_timer_stop(wait_beacon_timeout_timer);
                mac_state_transfer(DE_MAC_STATES_INITED);
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_START_JOINING);

                return signal ^ SIGNAL_MAC_ENGINE_LOST_BEACON;
            }
            else {
                return 0;
            }
        case DE_JOINED_STATES_RX_FRAME:
            // todo
            return 0;
        case DE_JOINED_STATES_TX_FRAME:
            /*
             * Has tracked beacon, and wait till the right section (i.e. right
             * contention field). Ready to send frames.
             * todo
             */
            if (signal & SIGNAL_MAC_ENGINE_SEND_FRAME) {
                _lpwan_debug_mark_running_line();

                static os_uint32 _span;
                static os_uint32 _delay;
                
                _span = (_s_info->beacon_section_length_us *
                        (64 - mac_info.allow_tx_info.allow_msg_emergent))
                        / 1000;
                _delay = hdk_randr(5, _span-50);

                haddock_assert(timer_not_started(_timeout_timer));
                tx_timer = _timeout_timer;
                _lpwan_debug_mark_ling_timer_start();
                os_timer_reconfig(tx_timer, this->_pid,
                                  SIGNAL_MAC_ENGINE_SEND_FRAME_CCA,
                                  _delay);
                os_timer_start(tx_timer);

                if (_delay >= DEVICE_MAC_SLEEP_NEXT_TIMER_LENGTH_MS) {
                    process_sleep();
                }

                return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
            }
            else if (signal & SIGNAL_MAC_ENGINE_SEND_FRAME_CCA) {
                _lpwan_debug_mark_running_line();

                _channel_backoff_num = 0;
                lpwan_radio_stop_rx();
                lpwan_radio_start_rx();
                return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME_CCA;
            }
            else if (signal & SIGNAL_LPWAN_RADIO_RX_TIMEOUT) {
                _lpwan_debug_mark_running_line();

                static struct list_head *pos;
                static struct tx_frame_buffer *buffer;

                if (mac_info.pending_list_len == 0) {
                    return signal ^ SIGNAL_LPWAN_RADIO_RX_TIMEOUT;
                }
                
                pos = mac_info.pending_frame_buffer_list.next;
                buffer = list_entry(pos, struct tx_frame_buffer, hdr);

                buffer->beacon_seq_id = _s_info->beacon_seq_id;
                buffer->expected_beacon_seq_id = \
                        get_expected_beacon_seq_id(_s_info->beacon_seq_id,
                                                   (os_int8) _s_info->packed_ack_delay_num);
                if (buffer->transmit_times == 0) {
                    // a newly tx frame
                    buffer->seq = ++ mac_info.tx_frame_seq_id;
                }
                buffer->transmit_times += 1;
                buffer->channel_backoff_num = _channel_backoff_num;

                if (buffer->ftype == FTYPE_DEVICE_MSG) {
                    _lpwan_debug_mark_running_line();
                    update_device_tx_frame_msg(
                            (void *) & buffer->frame_buffer[FRAME_HDR_LEN_NORMAL],
                            buffer,
                            _s_info->beacon_group_seq_id,
                            _s_info->beacon_class_seq_id);

                    lpwan_radio_tx(buffer->frame_buffer, buffer->len);
                    print_debug_str("sn msg: radio_tx!");
                } else {
                    _lpwan_debug_mark_running_line();
                    /*
                     * todo we have only FTYPE_DEVICE_MSG currently.
                     */
                }

                list_move_tail(pos, & mac_info.wait_ack_frame_buffer_list);
                mac_info.pending_list_len -= 1;
                mac_info.wait_ack_list_len += 1;

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                return signal ^ SIGNAL_LPWAN_RADIO_RX_TIMEOUT;
            }
            else if (signal & SIGNAL_LPWAN_RADIO_RX_OK) {
                _lpwan_debug_mark_running_line();

                // discard whatever received ... tx next time. todo
                ++_channel_backoff_num;

                process_sleep();
                return signal ^ SIGNAL_LPWAN_RADIO_RX_OK;
            }
            else if (signal & SIGNAL_LPWAN_RADIO_TX_OK) {
                _lpwan_debug_mark_running_line();
                process_sleep();
                return signal ^ SIGNAL_LPWAN_RADIO_TX_OK;
            }
            else if (signal & SIGNAL_LPWAN_RADIO_TX_TIMEOUT) {
                _lpwan_debug_mark_running_line();
                process_sleep();
                return signal ^ SIGNAL_LPWAN_RADIO_TX_TIMEOUT;
            }
            else {
                _lpwan_debug_mark_running_line();
                return 0;
            }
        default:
            return 0;
        }
        break;
    case DE_MAC_STATES_LEAVING:
        break;
    case DE_MAC_STATES_LEFT:
        break;
    case DE_MAC_STATES_FORCE_LEFT:
        break;
    default:
        _lpwan_debug_mark_running_line();
        __should_never_fall_here();
    }

    // unknown signal? discard.
    return 0;
}

/*---------------------------------------------------------------------------*/
/**< Static functions related to state transfer and state handling etc. @{ */

static inline void mac_state_transfer(enum device_mac_states state)
{
    mac_info.mac_engine_states = state;
}

static inline void mac_joining_state_transfer(enum device_mac_joining_states state)
{
    mac_info.joining_states = state;
}

static inline void mac_joined_state_transfer(enum device_mac_joined_states state)
{
    mac_info.joined_states = state;
}

/**
 * return positive value if valid beacon found. else return -1.
 * \remark Make sure to run device_mac_get_uuid() and device_mac_calculate_suuid() first.
 */
static os_int8 rx_and_see_is_it_a_beacon(os_uint8 *rx_buf, os_uint8 len,
                    struct parsed_frame_hdr_info *frame_hdr_info,
                    struct parsed_beacon_info *beacon_info,
                    struct parsed_beacon_packed_ack_to_me *beacon_packed_ack,
                    struct parsed_beacon_op2_to_me *beacon_op2)
{
    static os_boolean _run_first_time = OS_TRUE;
    static struct parse_beacon_check_op2_ack_info check_info;

    _lpwan_debug_mark_running_line();

    if (_run_first_time) {
        check_info.uuid = mac_info.uuid;
        check_info.suuid = mac_info.suuid;
        _run_first_time = OS_FALSE;
    }

    check_info.is_check_op2 = mac_info.is_check_op2;
    check_info.is_check_packed_ack = mac_info.is_check_packed_ack;
    check_info.short_addr = mac_info.short_addr;

    lpwan_radio_read(rx_buf, len);

    os_int8 _len = -1;
    if (rx_buf[0] > 0) {
        _len = lpwan_parse_frame_header((void *) (rx_buf+1), rx_buf[0],
                                        frame_hdr_info);
        if (_len > 0
            && frame_hdr_info->frame_origin_type == DEVICE_GATEWAY
            && frame_hdr_info->info.gw.frame_type == FTYPE_GW_BEACON)
        {
            haddock_assert(_len <= rx_buf[0]);
            _len = lpwan_parse_beacon(rx_buf+1+_len, rx_buf[0]-_len,
                                      & check_info,
                                      beacon_info,
                                      beacon_packed_ack,
                                      beacon_op2);
        } else {
            _len = -1;
        }
    }
    return _len;
}

static void update_synced_beacon_info(struct parsed_frame_hdr_info *f_hdr,
                                      struct parsed_beacon_info *info,
                                      struct parsed_beacon_packed_ack_to_me *ack,
                                      struct parsed_beacon_op2_to_me *op2)
{
    _lpwan_debug_mark_running_line();

    mac_info.synced_beacon_info = *info;
    mac_info.synced_beacon_packed_ack_to_me = *ack;
    mac_info.synced_beacon_op2_to_me = *op2;

    haddock_get_time_tick_now(& mac_info.beacon_sync_time);

    mac_info.expected_beacon_seq_id = info->beacon_seq_id;
    mac_info.expected_beacon_classes_num = info->beacon_classes_num;
    mac_info.expected_class_seq_id = info->beacon_class_seq_id;

    mac_info.is_beacon_synchronized = OS_TRUE;

    // todo
    mac_info.gateway_cluster_addr = f_hdr->src.addr.short_addr;
    mac_info.synced_beacon_info.gateway_cluster_addr = mac_info.gateway_cluster_addr;

    struct allow_tx_info *_tx_info = & mac_info.allow_tx_info;
    if (mac_info.mac_engine_states >= DE_MAC_STATES_JOINED) {

        _tx_info->allow_msg_emergent = \
                info->ratio.ratio_op1 +
                info->ratio.ratio_op2;

        if (((os_uint16) mac_info.short_addr) % info->beacon_class_seq_id != 0) {
            _tx_info->allow_tx_cf23 = OS_FALSE;
        } else {
            _tx_info->allow_tx_cf23 = OS_TRUE;

            _tx_info->allow_msg_event    = -1;
            _tx_info->allow_msg_routine  = -1;
            _tx_info->allow_cmd          = -1;
            /**
             * \sa beacon_period_section_ratio_t
             * todo
             * we don't account into the info->ratio.ratio_beacon currently.
             */
            if (info->ratio.ratio_cf2 > 0) {
                _tx_info->allow_msg_event = \
                        info->ratio.ratio_op1 +
                        info->ratio.ratio_op2 +
                        info->ratio.ratio_cf1;
            }
            if (!info->is_cf3_only_cmd && info->ratio.ratio_cf3 > 0) {
                _tx_info->allow_msg_routine = \
                        info->ratio.ratio_op1 +
                        info->ratio.ratio_op2 +
                        info->ratio.ratio_cf1 +
                        info->ratio.ratio_cf2;
            }

            if (info->is_cf3_only_cmd) {
                if (_tx_info->allow_msg_event > 0)
                    _tx_info->allow_cmd = _tx_info->allow_msg_event;
                else if (info->ratio.ratio_cf3 > 0)
                    _tx_info->allow_cmd = \
                        info->ratio.ratio_op1 +
                        info->ratio.ratio_op2 +
                        info->ratio.ratio_cf1 +
                        info->ratio.ratio_cf2;
            }
        }
    }
}

static inline os_int8 get_expected_beacon_seq_id(os_int8 cur_seq_id,
                                                 os_int8 packed_ack_delay_num)
{
    haddock_assert(packed_ack_delay_num > 0);
    if (cur_seq_id < (os_int8) (BEACON_MAX_SEQ_NUM - packed_ack_delay_num))
        return cur_seq_id + packed_ack_delay_num;
    else
        return packed_ack_delay_num - (BEACON_MAX_SEQ_NUM - cur_seq_id);
}

/**
 * Update the tx message (emergent/event/routine messages) of end devices.
 * \remark We obtain the transmission sequence @seq and beacon sequence id
 *         @beacon_seq_id from @buffer.
 */
static void update_device_tx_frame_msg(struct device_uplink_msg *msg,
                                       const struct tx_frame_buffer *buffer,
                                       os_uint8 beacon_group_seq_id,
                                       os_uint8 beacon_class_seq_id)
{
    _lpwan_debug_mark_running_line();

    haddock_assert(beacon_group_seq_id <= 15);
    haddock_assert(beacon_class_seq_id <= 15);

    os_uint8 retransmit_num = buffer->transmit_times - 1;
    os_uint8 channel_backoff_num = buffer->channel_backoff_num - 1;

    retransmit_num = (retransmit_num > LPWAN_MAX_RETRANSMIT_NUM) ?
                      LPWAN_MAX_RETRANSMIT_NUM : retransmit_num;
    channel_backoff_num = (channel_backoff_num > LPWAN_MAX_CHANNEL_BACKOFF_NUM) ?
                           LPWAN_MAX_CHANNEL_BACKOFF_NUM : channel_backoff_num;

    /**< todo \sa struct device_uplink_common::hdr */
    os_uint8 _tx_repeat_info = (retransmit_num << 3) + channel_backoff_num;
    msg->hdr.hdr &= ((((os_uint8)1)<<5)-1);
    msg->hdr.hdr |= _tx_repeat_info;

    msg->seq = buffer->seq;

    msg->hdr.beacon_lqi = 255;
    msg->hdr.beacon_rssi = -1;

    os_uint8 *_beacon_seq = (os_uint8 *) msg->hdr.beacon_seq;
    _beacon_seq[0] = (os_uint8) buffer->beacon_seq_id;
    _beacon_seq[1] = (beacon_group_seq_id << 4) + beacon_class_seq_id;
}

/**< @} */
/*---------------------------------------------------------------------------*/
/**< MAC engine wake up init and sleep cleanup callback functions. @{ */

static void mac_engine_wakeup_init_callback(void)
{
    lpwan_radio_wakeup();
    // we start the lpwan_radio_routine() and the periodical timer @radio_check_timer
    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_CHECK_RADIO_TIMEOUT);
}

static os_int8 mac_engine_sleep_cleanup_callback(void)
{
    lpwan_radio_sleep();
    os_timer_stop(radio_check_timer);

    return 0;
}

/**< @} */
/*---------------------------------------------------------------------------*/

static void device_mac_get_uuid(modem_uuid_t *uuid)
{
    // todo
}

/**
 * \remark make sure to call device_mac_get_uuid() first.
 */
static void device_mac_calculate_suuid(void)
{
    mac_info.suuid = (((os_uint16) mac_info.uuid.addr[7]) << 8)
                     + mac_info.uuid.addr[6];
}

/**
 * Call device_mac_get_uuid() first.
 */
static void device_mac_srand(void)
{
    os_int32 seed = construct_u32_4(mac_info.uuid.addr[7],
                                    mac_info.uuid.addr[6],
                                    mac_info.uuid.addr[5],
                                    mac_info.uuid.addr[4]);
    hdk_srand(seed);
}

