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
#include "simple_log.h"

#include "frame_defs/frame_defs.h"

#include "lpwan_config.h"
#include "end_device/config_end_device.h"
#include "mac_engine_config.h"

#include "hal/hal_mcu.h"
#include "radio_config.h"
#include "radio_controller/radio_controller.h"

#include "protocol_utils/parse_frame_hdr.h"
#include "protocol_utils/parse_beacon.h"
#include "protocol_utils/construct_frame_hdr.h"
#include "protocol_utils/construct_de_uplink_msg.h"

#include "lib_util/crc/crc.h"

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
    os_uint8 seq;
    os_int8 beacon_seq_id;          /**< the beacon sequence id when the message is sent. */
    os_int8 expected_beacon_seq_id; /**< the expected beacon seq id to received ACK. */

    enum frame_type_end_device ftype;
    enum device_message_type msg_type;  /**< if @ftype is FTYPE_DEVICE_MSG, use
                                           this @msg_type to decide its type. */
    os_uint8 len;
    os_uint8 frame[LPWAN_DEVICE_MAC_UPLINK_MTU] __attribute__((aligned (4)));
};

/**
 * \sa LPWAN_DEVICE_MAC_UPLINK_BUFFER_MAX_NUM
 * \sa sizeof(struct tx_frame_buffer)
 */
static struct mem_pool_hdr *_uplink_frame_pool;

#define DE_MAC_SHORT_ADDR_INVALID   0x0000
static struct lpwan_device_mac_info mac_info __attribute__((aligned (4)));

/** 2 alias(es) */
struct parsed_beacon_info
    *_s_info = & mac_info.synced_beacon_info;
static struct parsed_beacon_packed_ack_to_me
    *_s_ack = & mac_info.synced_beacon_packed_ack_to_me;

/**< @} */
/*---------------------------------------------------------------------------*/
/**< static function declarations @{ */

static signal_bv_t device_mac_engine_entry(os_pid_t pid, signal_bv_t signal);
static inline void mac_state_transfer(enum device_mac_states state);
static inline void mac_joining_state_transfer(enum device_mac_joining_states state);
static inline void mac_joined_state_transfer(enum device_mac_joined_states state);

static struct tx_frame_buffer *_alloc_tx_frame_buffer(void);
static void _free_tx_frame_buffer(struct tx_frame_buffer *buffer);

static void device_mac_check_packed_ack(os_boolean is_need_check_ack,
                                        os_boolean beacon_has_packed_ack,
                                        os_boolean beacon_has_ack_to_me);

static os_int8 rx_handler_is_it_a_beacon(const os_uint8 *rx_buf, os_uint8 len,
                    struct parsed_frame_hdr_info *frame_hdr_info,
                    struct parsed_beacon_info *beacon_info,
                    struct parsed_beacon_packed_ack_to_me *beacon_packed_ack);

static void update_synced_beacon_info(struct parsed_frame_hdr_info *f_hdr,
                                      struct parsed_beacon_info *info,
                                      struct parsed_beacon_packed_ack_to_me *ack);

static inline os_int8 get_expected_beacon_seq_id(os_int8 cur_seq_id,
                                                 os_int8 packed_ack_delay_num);
static inline os_int8 beacon_seq_id_cmp(os_int8 seq1, os_int8 seq2);

static void update_device_tx_frame_msg(struct device_uplink_msg *msg,
                                       const struct tx_frame_buffer *buffer,
                                       os_uint8 beacon_group_seq_id,
                                       os_uint8 beacon_class_seq_id);

static void device_mac_get_uuid(modem_uuid_t *uuid);
static void device_mac_calculate_suuid(void);
static void device_mac_srand(void);

static void device_update_beacon_info(os_uint32 next_update_delta);

/**< @} */
/*---------------------------------------------------------------------------*/
/**< file-scope resources @{ */

/**
 * the destination and source of a frame.
 */
static struct lpwan_addr _frame_dest;
static struct lpwan_addr _frame_src;

/**
 * the MAC's rx buffer for radio's rx frames
 * \sa struct tx_frame_buffer::frame
 */
extern os_uint8 *radio_rx_buffer;

/**< @} */
/*---------------------------------------------------------------------------*/
/**< debug related variables @{ */
#define xLPWAN_MAC_DEBUG_ENABLE_MARK_RUNNING_LINE
#define xLPWAN_MAC_DEBUG_ENABLE_MARK_RUNNING_LINE_BLOCK
#define xLPWAN_MAC_DEBUG_ENABLE_MARK_LINE_TIMER_START

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
/**< MAC related timers. @{ */

static struct timer *_timeout_timer; /**< internal timeout timer
                                          \sa joining_timer; tx_timer */
static struct timer *update_beacon_timer; // try to track beacon periodically

/**
 * \remark The two timers below use @_timeout_timer.
 * Only one timer _should_ exist at any time.
 * \sa _timeout_timer
 */
static struct timer *joining_timer;
static struct timer *tx_timer;

/**< @} */
/*---------------------------------------------------------------------------*/

os_pid_t gl_device_mac_engine_pid;
haddock_process("device_mac_engine");

void device_mac_engine_init(os_uint8 priority)
{
    struct process *proc_mac_engine = process_create(device_mac_engine_entry,
                                                     priority);
    haddock_assert(proc_mac_engine);

    gl_device_mac_engine_pid = proc_mac_engine->_pid;

    haddock_memset(& mac_info, 0, sizeof(mac_info));

    /**< initialize the MAC timers @{ */
    _timeout_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                    DEVICE_MAC_ENGINE_MAX_TIMER_DELTA_MS);
    haddock_assert(_timeout_timer);

    update_beacon_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                    DEVICE_MAC_ENGINE_MAX_TIMER_DELTA_MS);
    haddock_assert(update_beacon_timer);
    /**< @} */

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

    device_mac_srand();

    mac_state_transfer(DE_MAC_STATES_INITED);
    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_INIT_FINISHED);

    radio_controller_register_mac_engine(gl_device_mac_engine_pid);
}

static signal_bv_t device_mac_engine_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);

    /** Parsed result for current downlink frames from gateway (beacon) */
    static struct parsed_frame_hdr_info             _frame_hdr_info;
    static struct parsed_beacon_info                _beacon_info;
    static struct parsed_beacon_packed_ack_to_me    _beacon_packed_ack;

    /** Is lost beacon now? (joined states) */
    static os_boolean _is_lost_beacon = OS_FALSE;

    /** \remark Only one single frame can be sent during a beacon period. */
    static struct {
        struct tx_frame_buffer *tx_frame;
    } _cur_tx_frame_info;

    if (signal & SIGNAL_MAC_ENGINE_UPDATE_BEACON_INFO) {
        haddock_assert(!_is_lost_beacon && mac_info.is_beacon_synchronized);
        device_update_beacon_info(1000 * _s_info->beacon_period_length);
        return signal ^ SIGNAL_MAC_ENGINE_UPDATE_BEACON_INFO;
    }

    // beacon tracker
    if ((mac_info.mac_engine_states == DE_MAC_STATES_JOINING
         && mac_info.joining_states == DE_JOINING_STATES_TRACKING_BEACON) ||
        (mac_info.mac_engine_states == DE_MAC_STATES_JOINED
         && mac_info.joined_states  == DE_JOINED_STATES_TRACKING_BEACON))
    {
        if (signal & SIGNAL_RLC_RX_OK) {
            os_int8 _len = rx_handler_is_it_a_beacon(radio_rx_buffer,
                                        1+LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                                        & _frame_hdr_info,
                                        & _beacon_info,
                                        & _beacon_packed_ack);
            if (_len > 0) {
                // valid beacon frame from gateway.
                radio_controller_rx_stop();
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_BEACON_FOUND);
            } else {
#ifndef LPWAN_DEBUG_ONLY_TRACK_BEACON
                print_log(LOG_INFO, "JD: rx not beacon");
#endif
            }
            return signal ^ SIGNAL_RLC_RX_OK;
        }
        else if (signal & SIGNAL_RLC_RX_DURATION_TIMEOUT) {
            // has not tracked beacon.
#ifdef LPWAN_DEBUG_ONLY_TRACK_BEACON
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_START_JOINING);
#else
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_TRACK_BEACON_TIMEOUT);
#endif
            return signal ^ SIGNAL_RLC_RX_DURATION_TIMEOUT;
        }
        else {
            // do nothing, continue to handle the signals.
        }
    }

    switch ((int) mac_info.mac_engine_states) {
    case DE_MAC_STATES_DEFAULT:
        // don't care the @signal, discard.
        return 0;
    case DE_MAC_STATES_INITED:
        if (signal & SIGNAL_MAC_ENGINE_INIT_FINISHED) {
            mac_state_transfer(DE_MAC_STATES_JOINING);
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_DELAYED_JOINING);
            print_log(LOG_INFO, "init: => joining");
            return signal ^ SIGNAL_MAC_ENGINE_INIT_FINISHED;
        }
        else {
            return 0;
        }
    case DE_MAC_STATES_JOINING:
        if (signal & SIGNAL_MAC_ENGINE_DELAYED_JOINING) {
            mac_joining_state_transfer(DE_JOINING_STATES_DELAY_TO_TRACK_BEACON);
            // delay some time and perform joining
            haddock_assert(timer_not_started(_timeout_timer));
            joining_timer = _timeout_timer;

            os_uint32 _delay = hdk_randr(0, 1000) +
                               1000 * hdk_randr(LPWAN_MAC_JOIN_AFTER_INITED_MIN,
                                                LPWAN_MAC_JOIN_AFTER_INITED_MAX);
            os_timer_reconfig(joining_timer,
                              this->_pid, SIGNAL_MAC_ENGINE_START_JOINING, _delay);
            os_timer_start(joining_timer);
            print_log(LOG_INFO, "JI: (delay %lds:%dms)", _delay / 1000, _delay % 1000);

            process_sleep();
            return signal ^ SIGNAL_MAC_ENGINE_DELAYED_JOINING;
        }
        else if (signal & SIGNAL_MAC_ENGINE_START_JOINING) {
            // start joining process (track beacon, send join request ...)
            mac_joining_state_transfer(DE_JOINING_STATES_TRACKING_BEACON);

            print_log(LOG_INFO, "JI: @{ (%lds:%ldms)",
                      ((os_uint32) DEVICE_JOINING_FIND_BEACON_TIMEOUT_MS) / 1000,
                      ((os_uint32) DEVICE_JOINING_FIND_BEACON_TIMEOUT_MS) % 1000);
            radio_controller_rx_continuously(DEVICE_JOINING_FIND_BEACON_TIMEOUT_MS);

            return signal ^ SIGNAL_MAC_ENGINE_START_JOINING;
        }
        else if (signal & SIGNAL_MAC_ENGINE_TRACK_BEACON_TIMEOUT) {
            // Track beacon failed when joining. Restart the track process. todo
            print_log(LOG_WARNING, "JI: @} timeout");
            mac_joining_state_transfer(DE_JOINING_STATES_TRACKING_BEACON_TIMEOUT);
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_DELAYED_JOINING);

            return signal ^ SIGNAL_MAC_ENGINE_TRACK_BEACON_TIMEOUT;
        }
        else if (signal & SIGNAL_MAC_ENGINE_BEACON_FOUND) {
            print_log(LOG_INFO, "JI: @} (%d)", _beacon_info.beacon_seq_id);
            mac_joining_state_transfer(DE_JOINING_STATES_BEACON_FOUND);

#ifdef LPWAN_DEBUG_ONLY_TRACK_BEACON
            // We only track beacon. Don't go to joined states, stay within joining states.
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_START_JOINING);
#else
            if (_beacon_info.is_join_allowed || _is_lost_beacon)
            {
                // set up the periodically timer to track beacon
                os_uint32 _delta = 1000 * _beacon_info.beacon_period_length -
                                   DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS;
                os_timer_reconfig(update_beacon_timer, this->_pid,
                                  SIGNAL_MAC_ENGINE_UPDATE_BEACON_INFO, _delta);
                os_timer_start(update_beacon_timer);

                update_synced_beacon_info(& _frame_hdr_info,
                                          & _beacon_info,
                                          & _beacon_packed_ack);

                if (_is_lost_beacon) {
                    // has joined previously, no need to send join request again.
                    _is_lost_beacon = OS_FALSE;
                    mac_state_transfer(DE_MAC_STATES_JOINED);
                    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                    process_sleep();
                } else {
                    // a new join
                    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST);
                }
            } else {
                // the gateway cluster don't allow joining.
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_DELAYED_JOINING);
            }
#endif
            return signal ^ SIGNAL_MAC_ENGINE_BEACON_FOUND;
        }
        else if (signal & SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST) {
            /*
             * we simply generate short_addr from modem_uuid currently. todo
             */
            mac_info.short_addr = (os_uint16) mac_info.suuid;
            mac_info.tx_frame_seq_id = (os_uint8) hdk_randr(0, 255);
            mac_state_transfer(DE_MAC_STATES_JOINED);
            mac_joined_state_transfer(DE_JOINED_STATES_IDLE);

            print_log(LOG_INFO, "JI: => joined");

            process_sleep();
            return signal ^ SIGNAL_MAC_ENGINE_SEND_JOIN_REQUEST;
        }
        else {
            return 0;
        }
    case DE_MAC_STATES_JOINED:
        switch ((int) mac_info.joined_states) {
        case DE_JOINED_STATES_IDLE:
            if (signal & SIGNAL_MAC_ENGINE_TRACK_BEACON) {
                mac_joined_state_transfer(DE_JOINED_STATES_TRACKING_BEACON);
                print_log(LOG_INFO, "JD: %04x @{ (%lds:%ldms)",
                          (os_uint16) mac_info.short_addr,
                          ((os_uint32) DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS) / 1000,
                          ((os_uint32) DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS) % 1000);

                radio_controller_rx_continuously(DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS);

                return signal ^ SIGNAL_MAC_ENGINE_TRACK_BEACON;
            }
            else {
                return 0;
            }
        case DE_JOINED_STATES_TRACKING_BEACON:
            if (signal & SIGNAL_MAC_ENGINE_BEACON_FOUND) {
                update_synced_beacon_info(& _frame_hdr_info,
                                          & _beacon_info,
                                          & _beacon_packed_ack);

                if (_s_info->beacon_seq_id == mac_info.expected_beacon_seq_id
                    && _s_info->beacon_classes_num == mac_info.expected_beacon_classes_num
                    && _s_info->beacon_class_seq_id == mac_info.expected_class_seq_id)
                {
                    // set up the periodically timer to track beacon
                    os_uint32 _delta = 1000 * _beacon_info.beacon_period_length -
                                       DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS;
                    os_timer_stop(update_beacon_timer);
                    os_timer_reconfig(update_beacon_timer, this->_pid,
                                      SIGNAL_MAC_ENGINE_UPDATE_BEACON_INFO, _delta);
                    os_timer_start(update_beacon_timer);

                    print_log(LOG_INFO, "JD: @} (%d)", _s_info->beacon_seq_id);

                    device_mac_check_packed_ack(mac_info.is_check_packed_ack,
                                                _s_info->has_packed_ack,
                                                _s_ack->has_ack);

                    // check downlink messages todo

                    if ((! list_empty(& mac_info.pending_frame_buffer_list))
                        && _s_info->ratio.ratio_uplink_msg > DEVICE_MAC_MIN_CF_RATIO)
                    {
                        static os_uint32 _delay;
                        _delay = (_s_info->beacon_section_length_us * _s_info->ratio.ratio_downlink_msg)
                                 / 1000;

                        mac_joined_state_transfer(DE_JOINED_STATES_TX_FRAME);

                        if (_delay == 0) {
                            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_SEND_FRAME);
                        } else {
                            haddock_assert(timer_not_started(_timeout_timer));
                            tx_timer = _timeout_timer;
                            os_timer_reconfig(tx_timer, this->_pid,
                                              SIGNAL_MAC_ENGINE_SEND_FRAME, _delay);
                            os_timer_start(tx_timer);

                            if (_delay >= DEVICE_MAC_SLEEP_NEXT_TIMER_LENGTH_MS) {
                                process_sleep();
                            }
                        }
                    } else {
                        mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                    }
                }
                else {
                    // we regard it as lost beacon todo
                    print_log(LOG_WARNING, "JD: @} mis-match (E:%d-%d-%d; R:%d-%d-%d)",
                                      mac_info.expected_beacon_seq_id,
                                      mac_info.expected_beacon_classes_num,
                                      mac_info.expected_class_seq_id,
                                      _s_info->beacon_seq_id,
                                      _s_info->beacon_classes_num,
                                      _s_info->beacon_class_seq_id);
                    os_timer_stop(update_beacon_timer);
                    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_TRACK_BEACON_TIMEOUT);
                }
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_FOUND;
            }
            else if (signal & SIGNAL_MAC_ENGINE_TRACK_BEACON_TIMEOUT) {
                print_log(LOG_WARNING, "JD: @} timeout");
                os_timer_stop(update_beacon_timer);
                _is_lost_beacon = OS_TRUE;

                mac_joined_state_transfer(DE_JOINED_STATES_TRACKING_BEACON_TIMEOUT);
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_LOST_BEACON);

                return signal ^ SIGNAL_MAC_ENGINE_TRACK_BEACON_TIMEOUT;
            }
            else {
                return 0;
            }
        case DE_JOINED_STATES_TRACKING_BEACON_TIMEOUT:
            if (signal & SIGNAL_MAC_ENGINE_LOST_BEACON) {
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

                // re-start tracking beacon
                mac_state_transfer(DE_MAC_STATES_JOINING);
                os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_START_JOINING);

                return signal ^ SIGNAL_MAC_ENGINE_LOST_BEACON;
            } else {
                return 0;
            }
            break;
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
                static os_uint32 _span;
                static os_uint32 _try_tx_duration;

                if (mac_info.pending_list_len == 0) {
                    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                    return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
                }

                struct list_head *pos;
                pos = mac_info.pending_frame_buffer_list.next;
                _cur_tx_frame_info.tx_frame = list_entry(pos, struct tx_frame_buffer, hdr);

                _cur_tx_frame_info.tx_frame->beacon_seq_id = _s_info->beacon_seq_id;
                _cur_tx_frame_info.tx_frame->expected_beacon_seq_id = \
                        get_expected_beacon_seq_id(_s_info->beacon_seq_id,
                                                   (os_int8) _s_info->packed_ack_delay_num);
                if (_cur_tx_frame_info.tx_frame->transmit_times == 0) {
                    // a newly tx frame
                    _cur_tx_frame_info.tx_frame->seq = ++ mac_info.tx_frame_seq_id;
                }
                _cur_tx_frame_info.tx_frame->transmit_times += 1;

                switch ((int) _cur_tx_frame_info.tx_frame->ftype) {
                case FTYPE_DEVICE_MSG           :
                    update_device_tx_frame_msg(
                            (void *) & _cur_tx_frame_info.tx_frame->frame[FRAME_HDR_LEN_NORMAL],
                            _cur_tx_frame_info.tx_frame,
                            _s_info->beacon_group_seq_id,
                            _s_info->beacon_class_seq_id);
                    print_log(LOG_INFO, "JD: rlc_tx (%s %dB %dT) >%d",
                              device_msg_type_string[(int) _cur_tx_frame_info.tx_frame->msg_type],
                              _cur_tx_frame_info.tx_frame->len,
                              _cur_tx_frame_info.tx_frame->transmit_times,
                              _cur_tx_frame_info.tx_frame->seq);
                    break;
                case FTYPE_DEVICE_JOIN          :
                case FTYPE_DEVICE_REJOIN        :
                case FTYPE_DEVICE_DATA_REQUEST  :
                case FTYPE_DEVICE_ACK           :
                case FTYPE_DEVICE_CMD           :
                    /*
                     * todo we have only FTYPE_DEVICE_MSG currently.
                     */
                    break;
                default:
                    __should_never_fall_here();
                    break;
                }

                os_uint8 _n_slots = _s_info->ratio.ratio_uplink_msg;
                _span = (_s_info->beacon_section_length_us * (_n_slots)) / 1000;
                _try_tx_duration = hdk_randr(RADIO_CONTROLLER_MIN_TRY_TX_DURATION,
                                             _span-100); // to avoid conflict todo

                radio_controller_tx(_cur_tx_frame_info.tx_frame->frame,
                                    _cur_tx_frame_info.tx_frame->len, _try_tx_duration);

                if (_try_tx_duration >= DEVICE_MAC_SLEEP_NEXT_TIMER_LENGTH_MS) {
                    process_sleep();
                }

                return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
            }
            else if (signal & SIGNAL_RLC_TX_OK) {
                print_log(LOG_INFO, "JD: rlc_tx ok >%d",
                          _cur_tx_frame_info.tx_frame->seq);

                list_move_tail(& _cur_tx_frame_info.tx_frame->hdr,
                               & mac_info.wait_ack_frame_buffer_list);
                mac_info.pending_list_len -= 1;
                mac_info.wait_ack_list_len += 1;

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                process_sleep();
                return signal ^ SIGNAL_RLC_TX_OK;
            }
            else if (signal & SIGNAL_RLC_TX_CCA_FAILED) {
                // discard whatever received ... tx next time. todo
                print_log(LOG_WARNING, "JD: rlc_tx CCA fail >%d",
                                  _cur_tx_frame_info.tx_frame->seq);

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                process_sleep();
                return signal ^ SIGNAL_RLC_TX_CCA_FAILED;
            }
            else if (signal & SIGNAL_RLC_TX_CCA_CRC_FAIL) {
                // we regard it as CCA failed too (tx next time).
                print_log(LOG_WARNING, "JD: rlc_tx CCA CRC fail >%d",
                                  _cur_tx_frame_info.tx_frame->seq);

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                process_sleep();
                return signal ^ SIGNAL_RLC_TX_CCA_CRC_FAIL;
            }
            else if (signal & SIGNAL_RLC_TX_TIMEOUT) {
                print_log(LOG_WARNING, "JD: rlc_tx timeout >%d",
                                  _cur_tx_frame_info.tx_frame->seq);

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                process_sleep();
                return signal ^ SIGNAL_RLC_TX_TIMEOUT;
            }
            else {
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
static os_int8 rx_handler_is_it_a_beacon(const os_uint8 *rx_buf, os_uint8 len,
                    struct parsed_frame_hdr_info *frame_hdr_info,
                    struct parsed_beacon_info *beacon_info,
                    struct parsed_beacon_packed_ack_to_me *beacon_packed_ack)
{
    static os_boolean _run_first_time = OS_TRUE;
    static struct parse_beacon_check_info check_info;

    if (_run_first_time) {
        check_info.uuid = mac_info.uuid;
        check_info.suuid = mac_info.suuid;
        _run_first_time = OS_FALSE;
    }

    check_info.is_check_packed_ack = mac_info.is_check_packed_ack;
    check_info.short_addr = mac_info.short_addr;

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
                                      beacon_packed_ack);
        } else {
            _len = -1;
        }
    }
    return _len;
}

static void update_synced_beacon_info(struct parsed_frame_hdr_info *f_hdr,
                                      struct parsed_beacon_info *info,
                                      struct parsed_beacon_packed_ack_to_me *ack)
{
    mac_info.synced_beacon_info = *info;
    mac_info.synced_beacon_packed_ack_to_me = *ack;

    haddock_get_time_tick_now(& mac_info.beacon_sync_time);

    mac_info.expected_beacon_seq_id = info->beacon_seq_id;
    mac_info.expected_beacon_classes_num = info->beacon_classes_num;
    mac_info.expected_class_seq_id = info->beacon_class_seq_id;

    mac_info.is_beacon_synchronized = OS_TRUE;

    // todo
    mac_info.gateway_cluster_addr = f_hdr->src.addr.short_addr;
    mac_info.synced_beacon_info.gateway_cluster_addr = mac_info.gateway_cluster_addr;
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

/*
 * If seq1 is later(greater) than seq2, return 1;
 * else if equal, return 0;
 * else, return -1.
 */
static inline os_int8 beacon_seq_id_cmp(os_int8 seq1, os_int8 seq2)
{
    if (seq1 == seq2)
        return 0;
    else if (seq1 > seq2 && (seq1-seq2) < (BEACON_MAX_SEQ_NUM/2))
        return 1;
    else
        return -1;
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
    haddock_assert(beacon_group_seq_id <= 15);
    haddock_assert(beacon_class_seq_id <= 15);

    os_uint8 retransmit_num = buffer->transmit_times - 1;
    os_uint8 channel_backoff_num = 0; // todo

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

static void device_mac_get_uuid(modem_uuid_t *uuid)
{
    os_uint8 *_mcu_unique_id = (os_uint8 *) 0x4926;
    // os_uint8 _crc = CRC_Calc(POLY_CRC8_CCITT, _mcu_unique_id, 12);

    uuid->addr[0] = _mcu_unique_id[11];
    uuid->addr[1] = _mcu_unique_id[10];
    uuid->addr[2] = _mcu_unique_id[9];
    uuid->addr[3] = _mcu_unique_id[8];
    uuid->addr[4] = _mcu_unique_id[7];
    uuid->addr[5] = _mcu_unique_id[6];
    uuid->addr[6] = _mcu_unique_id[5];
    uuid->addr[7] = _mcu_unique_id[4];
    uuid->addr[8] = _mcu_unique_id[3];
    uuid->addr[9] = _mcu_unique_id[2];
    uuid->addr[10] = _mcu_unique_id[1];
    uuid->addr[11] = _mcu_unique_id[0];

    print_log(LOG_INFO, "init: uuid %02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x",
              uuid->addr[0], uuid->addr[1], uuid->addr[2], uuid->addr[3],
              uuid->addr[4], uuid->addr[5], uuid->addr[6], uuid->addr[7],
              uuid->addr[8], uuid->addr[9], uuid->addr[10], uuid->addr[11]);
}

/**
 * \remark make sure to call device_mac_get_uuid() first.
 */
static void device_mac_calculate_suuid(void)
{
    mac_info.suuid = short_modem_uuid(& mac_info.uuid);
    print_log(LOG_INFO, "init: suuid %04x", (os_uint16) mac_info.suuid);
}

/**
 * Call device_mac_get_uuid() first.
 */
static void device_mac_srand(void)
{
    os_int32 seed = construct_u32_4(91 + mac_info.uuid.addr[11],
                                    91 + mac_info.uuid.addr[10],
                                    91 + mac_info.uuid.addr[9],
                                    91 + mac_info.uuid.addr[8]);
    hdk_srand(seed);
}

/**
 * If need to track beacon, set signal @SIGNAL_MAC_ENGINE_TRACK_BEACON
 * Reconfig the @update_beacon_timer timer to periodically update beacon information.
 */
static void device_update_beacon_info(os_uint32 next_update_delta)
{
    mac_info.expected_beacon_seq_id += 1;
    if (mac_info.expected_beacon_seq_id == BEACON_MAX_SEQ_NUM) {
        mac_info.expected_beacon_seq_id = 0;
    }

    mac_info.expected_class_seq_id += 1;
    if (mac_info.expected_class_seq_id > _s_info->beacon_classes_num) {
        mac_info.expected_class_seq_id = 1;
    }

    mac_info.is_check_packed_ack = OS_FALSE;

    os_boolean need_track_beacon = OS_FALSE;

    if (mac_info.pending_list_len > 0) {
        if (((os_uint16) mac_info.short_addr) % mac_info.expected_class_seq_id == 0) {
            need_track_beacon = OS_TRUE;
        }
    }

    if (mac_info.wait_ack_list_len > 0) {
        struct list_head *pos;
        struct tx_frame_buffer *buf;

        list_for_each(pos, & mac_info.wait_ack_frame_buffer_list) {
            buf = list_entry(pos, struct tx_frame_buffer, hdr);
            if (buf->expected_beacon_seq_id == mac_info.expected_beacon_seq_id) {
                // we need to check ACK from gateway
                mac_info.is_check_packed_ack = OS_TRUE;
                need_track_beacon = OS_TRUE;
                break;
            }
        }
    }

#ifdef LPWAN_DEBUG_ONLY_TRACK_BEACON
    need_track_beacon = OS_TRUE;
#endif
    
    if (need_track_beacon) {
        os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_TRACK_BEACON);
    }

    os_timer_reconfig(update_beacon_timer, this->_pid,
                      SIGNAL_MAC_ENGINE_UPDATE_BEACON_INFO, next_update_delta);
    os_timer_start(update_beacon_timer);
}

static struct tx_frame_buffer *_alloc_tx_frame_buffer(void)
{
    struct mem_pool_blk *blk = \
            mem_pool_alloc_blk(_uplink_frame_pool, sizeof(struct tx_frame_buffer));
    return (blk == NULL) ? NULL : ((struct tx_frame_buffer*) blk->blk);
}

static void _free_tx_frame_buffer(struct tx_frame_buffer *buffer)
{
    struct mem_pool_blk *blk = find_mem_pool_blk(buffer);
    mem_pool_free_blk(blk);
}

/**
 * Check the packed ACK after receiving a matched beacon frame.
 */
static void device_mac_check_packed_ack(os_boolean is_need_check_ack,
                                        os_boolean beacon_has_packed_ack,
                                        os_boolean beacon_has_ack_to_me)
{
    struct list_head *pos;
    struct list_head *n;
    struct tx_frame_buffer *buffer;

    if (is_need_check_ack && beacon_has_packed_ack && beacon_has_ack_to_me) {
        list_for_each_safe(pos, n, & mac_info.wait_ack_frame_buffer_list) {
            buffer = list_entry(pos, struct tx_frame_buffer, hdr);
            if (buffer->expected_beacon_seq_id == _s_info->beacon_seq_id) {
                // todo
                print_log(LOG_INFO_COOL, "JD: rx (ACK) <%d", buffer->seq);

                list_del_init(pos);
                _free_tx_frame_buffer(buffer);
                mac_info.wait_ack_list_len -= 1;
            }
        }
    }
    else if (is_need_check_ack) {
        // need check ack but no ACK. todo

        os_int8 cmp_result;
        list_for_each_safe(pos, n, & mac_info.wait_ack_frame_buffer_list) {
            buffer = list_entry(pos, struct tx_frame_buffer, hdr);
            cmp_result = beacon_seq_id_cmp(_s_info->beacon_seq_id,
                                           buffer->expected_beacon_seq_id);
            haddock_assert(cmp_result != 1);

            if (cmp_result == 0) {
                if (buffer->transmit_times < LPWAN_MAX_RETRANSMIT_NUM) {
                    print_log(LOG_WARNING, "JD: rx (NE) %d retry", buffer->seq);
                    list_move_tail(pos, & mac_info.pending_frame_buffer_list);
                    mac_info.wait_ack_list_len -= 1;
                    mac_info.pending_list_len += 1;
                } else {
                    // too many re-transmissions, discard the packets.
                    print_log(LOG_WARNING, "JD: rx (NE) %d discard", buffer->seq);
                    list_del_init(pos);
                    _free_tx_frame_buffer(buffer);
                    mac_info.wait_ack_list_len -= 1;
                }
            }

        }
    }
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
        haddock_assert(timer_started(update_beacon_timer));
        print_log(LOG_WARNING, "JD: tx_buf (full)");
        return DEVICE_SEND_MSG_ERR_PENDING_TX_BUFFER_FULL;
    }

    struct tx_frame_buffer *frame_buffer = _alloc_tx_frame_buffer();
    if (! frame_buffer) {
        print_log(LOG_WARNING, "JD: tx_buf (alloc fail)");
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

    construct_device_frame_header(frame_buffer->frame, LPWAN_DEVICE_MAC_UPLINK_MTU,
                                  FTYPE_DEVICE_MSG,
                                  & _frame_src, & _frame_dest,
                                  OS_FALSE, // is_mobile?
                                  RADIO_TX_POWER_LEVELS_NUM-1);

    // construct the uplink message
    construct_device_uplink_msg(type, msg, len,
                                & frame_buffer->frame[FRAME_HDR_LEN_NORMAL],
                                LPWAN_DEVICE_MAC_UPLINK_MTU - FRAME_HDR_LEN_NORMAL);
    frame_buffer->len = FRAME_HDR_LEN_NORMAL + sizeof(struct device_uplink_msg) + len;

    list_add_tail(& frame_buffer->hdr, & mac_info.pending_frame_buffer_list);
    mac_info.pending_list_len += 1;
    
    print_log(LOG_INFO, "JD: tx_buf (%s F%dB P%dB)",
              device_msg_type_string[type], frame_buffer->len, len);

    return 0;
}

enum device_mac_states device_get_mac_states(void)
{
    return mac_info.mac_engine_states;
}
