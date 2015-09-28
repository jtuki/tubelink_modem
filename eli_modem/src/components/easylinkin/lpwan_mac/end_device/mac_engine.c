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

#include "beacon_tracker/beacon_tracker.h"
#include "tx_frames_mgmt/tx_frames_mgmt.h"

#include "mac_engine.h"

/*---------------------------------------------------------------------------*/
/**< MAC information base etc. @{ */
static struct lpwan_device_mac_info mac_info __attribute__((aligned (4)));

/** alias(es) */
static const struct parsed_beacon_info *gl_bcn_info;
static const struct parsed_beacon_packed_ack_to_me *gl_bcn_ack;
static const struct beacon_tracker_info_base *gl_btracker_info_base;
static const struct lpwan_last_rx_frame_time *gl_bcn_time;
static const struct lpwan_last_rx_frame_rssi_snr *gl_bcn_signal_strength;

/**< @} */
/*---------------------------------------------------------------------------*/
/**< static function declarations @{ */

static signal_bv_t device_mac_engine_entry(os_pid_t pid, signal_bv_t signal);
static inline void mac_state_transfer(enum device_mac_states state);
static inline void mac_joining_state_transfer(enum device_mac_joining_states state);
static inline void mac_joined_state_transfer(enum device_mac_joined_states state);

static inline os_int8 calc_expected_beacon_seq_id(os_int8 cur_seq_id,
                                                 os_int8 packed_ack_delay_num);

static void mac_engine_delayed_rx_downlink_frame(void);
static void mac_engine_delayed_tx_pending_frame(void);
static void mac_engine_do_tx_msg(const struct tx_fbuf *fbuf);

static void mac_engine_handle_tx_msg_fail(const struct tx_fbuf *fbuf, signal_bv_t signal);
static void mac_engine_handle_tx_msg_ok(const struct tx_fbuf *fbuf);

static void mac_engine_delayed_start_joining(os_uint32 delay_ms);
static void mac_engine_init_join_req(void);

static void device_mac_get_uuid(modem_uuid_t *uuid);
static void device_mac_calculate_suuid(void);
static void device_mac_srand(void);

/**< @} */
/*---------------------------------------------------------------------------*/

static struct timer *gl_timeout_timer; /**< internal timeout timer */

/** \remark Only one frame can be sent during a beacon period. */
static struct {
    const struct tx_fbuf *tx_frame;
} cur_tx;

os_pid_t gl_device_mac_engine_pid;
haddock_process("device_mac_engine");

void device_mac_engine_init(os_uint8 priority)
{
    struct process *proc_mac_engine = process_create(device_mac_engine_entry,
                                                     priority);
    haddock_assert(proc_mac_engine);

    gl_device_mac_engine_pid = proc_mac_engine->_pid;

    haddock_memset(& mac_info, 0, sizeof(mac_info));

    /**< initialize the MAC timers */
    gl_timeout_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG, ((os_uint32)1)<<31);
    haddock_assert(gl_timeout_timer);

    mac_tx_frames_mgmt_init();

    device_mac_get_uuid(& mac_info.uuid);
    device_mac_calculate_suuid();

    // generate the random seed.
    device_mac_srand();

    mac_state_transfer(DE_MAC_STATES_INITED);
    os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_INIT_FINISHED);

    btracker_register_mac_engine(gl_device_mac_engine_pid);

    gl_bcn_info = btracker_get_parsed_beacon_info();
    gl_bcn_ack = btracker_get_parsed_packed_ack();
    gl_btracker_info_base = btracker_get_info_base();
    gl_bcn_time = btacker_get_last_bcn_time();
    gl_bcn_signal_strength = btracker_get_last_bcn_signal_strength();
}

static signal_bv_t device_mac_engine_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);

    switch ((int) mac_info.mac_engine_states) {
    case DE_MAC_STATES_DEFAULT:
        // don't care the @signal, discard.
        return 0;
    case DE_MAC_STATES_INITED:
        if (signal & SIGNAL_MAC_ENGINE_INIT_FINISHED) {
            os_uint32 delay_ms = hdk_randr(0, 1000) +
                                   1000 * hdk_randr(LPWAN_MAC_JOIN_AFTER_INITED_MIN,
                                                    LPWAN_MAC_JOIN_AFTER_INITED_MAX);
            mac_engine_delayed_start_joining(delay_ms);

            print_log(LOG_INFO, "INIT: delayJI %lds:%dms",
                      delay_ms / 1000, delay_ms % 1000);
            return signal ^ SIGNAL_MAC_ENGINE_INIT_FINISHED;
        }

        if (signal & SIGNAL_MAC_ENGINE_START_JOINING) {
            mac_state_transfer(DE_MAC_STATES_JOINING);
            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_SEARCH_BEACON);
            return signal ^ SIGNAL_MAC_ENGINE_START_JOINING;
        }

        __should_never_fall_here();
        break;
    case DE_MAC_STATES_JOINING:
        if (signal & SIGNAL_MAC_ENGINE_SEARCH_BEACON) {
            // beginning of the joining process (search beacon, send join request ...)
            mac_joining_state_transfer(DE_JOINING_STATES_SEARCH_BEACON);

            btracker_search_beacon(DEVICE_JOINING_SEARCH_BCN_TIMEOUT_MS);

            print_log(LOG_INFO, "JI: @{ (%lds:%ldms)",
                      ((os_uint32) DEVICE_JOINING_SEARCH_BCN_TIMEOUT_MS) / 1000,
                      ((os_uint32) DEVICE_JOINING_SEARCH_BCN_TIMEOUT_MS) % 1000);

            return signal ^ SIGNAL_MAC_ENGINE_SEARCH_BEACON;
        }

        if (signal & SIGNAL_MAC_ENGINE_BEACON_NOT_FOUND) {
            mac_state_transfer(DE_MAC_STATES_INITED);
            mac_engine_delayed_start_joining(LPWAN_MAC_SEARCH_BCN_AGAIN_MS);

            print_log(LOG_WARNING, "JI: @} delayJI %lds:%dms",
                      LPWAN_MAC_SEARCH_BCN_AGAIN_MS / 1000, LPWAN_MAC_SEARCH_BCN_AGAIN_MS % 1000);
            return signal ^ SIGNAL_MAC_ENGINE_BEACON_NOT_FOUND;
        }

        if (signal & SIGNAL_MAC_ENGINE_BEACON_FOUND) {
            mac_joining_state_transfer(DE_JOINING_STATES_BEACON_FOUND);

            if (gl_bcn_info->is_join_allowed)
            {
                // valid beacon found which allows for joining.
                os_ipc_set_signal(gl_btracker_pid, SIGNAL_BTRACKER_BEGIN_BEACON_TRACKING);
                mac_engine_init_join_req();
                process_sleep();

                print_log(LOG_INFO, "JI: @} (%d)", gl_bcn_info->beacon_seq_id);
            } else {
                // valid beacon found but don't allow joining.
                btracker_reset();
                mac_state_transfer(DE_MAC_STATES_INITED);
                mac_engine_delayed_start_joining(LPWAN_MAC_SEARCH_BCN_AGAIN_MS);

                print_log(LOG_WARNING, "JI: @} delayJI %lds:%dms",
                          LPWAN_MAC_SEARCH_BCN_AGAIN_MS / 1000, LPWAN_MAC_SEARCH_BCN_AGAIN_MS % 1000);
            }
            return signal ^ SIGNAL_MAC_ENGINE_BEACON_FOUND;
        }

        if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACKED) {
            /**
             * beacon tracked, check if has JOIN_REQ, send it. todo
             */
            print_log(LOG_INFO, "JI: tracked (%d)", gl_bcn_info->beacon_seq_id);
            return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACKED;
        }

        if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON) {
            mac_tx_frames_handle_lost_beacon();
            return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON;
        }

        if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED) {
            mac_state_transfer(DE_MAC_STATES_INITED);
            mac_engine_delayed_start_joining(LPWAN_MAC_SEARCH_BCN_AGAIN_MS);

            print_log(LOG_WARNING, "JI: track failed. delayJI %lds:%dms",
                      LPWAN_MAC_SEARCH_BCN_AGAIN_MS / 1000, LPWAN_MAC_SEARCH_BCN_AGAIN_MS % 1000);
            return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED;
        }
        __should_never_fall_here();
        break;
    case DE_MAC_STATES_JOINED:
        switch ((int) mac_info.joined_states) {
        case DE_JOINED_STATES_IDLE:
            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACKED) {
                /** Check awaiting ACK */
                if (mac_tx_frames_has_waiting_ack(gl_bcn_info->beacon_seq_id)) {
                    if (gl_bcn_ack->has_ack) {
                        mac_tx_frames_handle_check_ack(gl_bcn_info->beacon_seq_id,
                                                       gl_bcn_ack->confirmed_seq);
                    } else {
                        mac_tx_frames_handle_no_ack(gl_bcn_info->beacon_seq_id);
                    }
                }

                /** Check awaiting downlink frames (uplink-triggered downlink frames)
                 *  and pending tx frames.
                 *
                 * \note If there is downlink frame available, we only try to get the
                 *       downlink frame, and leave pending tx frames to later beacon
                 *       periods. */
                if (gl_bcn_ack->has_ack && gl_bcn_ack->is_msg_pending) {
                    mac_engine_delayed_rx_downlink_frame();
                } else if (mac_tx_frames_has_pending_tx()
                           && gl_bcn_info->ratio.slots_uplink_msg >= DE_MAC_ENGINE_MIN_UPLINK_SLOTS) {
                    mac_engine_delayed_tx_pending_frame();
                } else {
                    // no downlink or pending uplink frames, remains in DE_JOINED_STATES_IDLE
                }

                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACKED;
            }

            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON) {
                mac_tx_frames_handle_lost_beacon();
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON;
            }

            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED) {
                mac_tx_frames_handle_lost_beacon();

                mac_state_transfer(DE_MAC_STATES_INITED);
                mac_engine_delayed_start_joining(LPWAN_MAC_SEARCH_BCN_AGAIN_MS);

                print_log(LOG_WARNING, "JI: @} delayJI %lds:%dms",
                          LPWAN_MAC_SEARCH_BCN_AGAIN_MS / 1000, LPWAN_MAC_SEARCH_BCN_AGAIN_MS % 1000);

                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED;
            }

            __should_never_fall_here();
            break;
        case DE_JOINED_STATES_RX_FRAME:
            // todo
            return 0;
        case DE_JOINED_STATES_TX_FRAME:
            if (signal & SIGNAL_MAC_ENGINE_SEND_FRAME) {
                if (! mac_tx_frames_has_pending_tx()
                    || (cur_tx.tx_frame = mac_tx_frames_get_msg()) == NULL) {
                    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                    return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
                }

                mac_engine_do_tx_msg(cur_tx.tx_frame);

                print_log(LOG_INFO, "JD: rlc_tx (%s %dB %dT %dTF) >%d",
                          device_msg_type_string[(int) cur_tx.tx_frame->msg_type],
                          cur_tx.tx_frame->len,
                          cur_tx.tx_frame->transmit_times,
                          cur_tx.tx_frame->tx_fail_times,
                          cur_tx.tx_frame->seq);
                return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
            }
            else if (signal & SIGNAL_RLC_TX_OK) {
                mac_engine_handle_tx_msg_ok(cur_tx.tx_frame);

                print_log(LOG_INFO, "JD: rlc_tx ok >%d",
                          cur_tx.tx_frame->seq);
                return signal ^ SIGNAL_RLC_TX_OK;
            }
            else if (signal & SIGNAL_RLC_TX_CCA_FAILED) {
                // discard whatever received ... try to tx in next beacon period.
                mac_engine_handle_tx_msg_fail(cur_tx.tx_frame, signal);
                return signal ^ SIGNAL_RLC_TX_CCA_FAILED;
            }
            else if (signal & SIGNAL_RLC_TX_CCA_CRC_FAIL) {
                // we regard it as CCA failed too (tx next time).
                mac_engine_handle_tx_msg_fail(cur_tx.tx_frame, signal);
                return signal ^ SIGNAL_RLC_TX_CCA_CRC_FAIL;
            }
            else if (signal & SIGNAL_RLC_TX_TIMEOUT) {
                mac_engine_handle_tx_msg_fail(cur_tx.tx_frame, signal);
                return signal ^ SIGNAL_RLC_TX_TIMEOUT;
            }
            __should_never_fall_here();
            break;
        default:
            return 0;
        }
        break;
    default:
        __should_never_fall_here();
    }

    // unknown signal? discard.
    return 0;
}

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

static inline os_int8 calc_expected_beacon_seq_id(os_int8 cur_seq_id,
                                                 os_int8 packed_ack_delay_num)
{
    haddock_assert(packed_ack_delay_num > 0);
    if (cur_seq_id <= (os_int8) (BEACON_MAX_SEQ_NUM - packed_ack_delay_num))
        return cur_seq_id + packed_ack_delay_num;
    else
        return packed_ack_delay_num - (BEACON_MAX_SEQ_NUM - cur_seq_id) - 1;
}

/** If there is pending downlink frame, prepare to receive it. */
static void mac_engine_delayed_rx_downlink_frame(void)
{
    haddock_assert(mac_info.mac_engine_states == DE_MAC_STATES_JOINED
                   && mac_info.joined_states == DE_JOINED_STATES_IDLE);

    if (! (gl_bcn_ack->has_ack && gl_bcn_ack->is_msg_pending))
        return;

    static os_uint32 bcn_span;
    static os_uint32 prev_downlink_span;

    static struct time bcn_to_now;
    static os_uint32 bcn_to_now_ms;

    static os_uint32 delay_ms;

    bcn_span = gl_bcn_info->beacon_section_length_us *
               gl_bcn_info->ratio.slots_beacon / 1000;
    prev_downlink_span = gl_bcn_info->beacon_section_length_us *
                         gl_bcn_ack->total_prev_downlink_slots / 1000;

    haddock_time_calc_delta_till_now(& gl_bcn_time->tx, & bcn_to_now);
    bcn_to_now_ms = (bcn_to_now.s * 1000) + bcn_to_now.ms;
    bcn_to_now_ms = (bcn_to_now_ms > bcn_span) ? bcn_span : bcn_to_now_ms;

    delay_ms = (bcn_span - bcn_to_now_ms) + prev_downlink_span;
    delay_ms = (delay_ms > DEVICE_MAC_RECV_DOWNLINK_IN_ADVANCE_MS) ?
               (delay_ms - DEVICE_MAC_RECV_DOWNLINK_IN_ADVANCE_MS) : 0;

    mac_joined_state_transfer(DE_JOINED_STATES_RX_FRAME);

    if (delay_ms == 0) {
        os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_RECV_FRAME);
    } else {
        os_timer_reconfig(gl_timeout_timer, this->_pid,
                          SIGNAL_MAC_ENGINE_RECV_FRAME, delay_ms);
        os_timer_start(gl_timeout_timer);

        if (delay_ms >= LPWAN_DE_SLEEP_MIN_NEXT_TIMER_LENGTH_MS) {
            process_sleep();
        }
    }
}

/** \note If no downlink frame, try to tx pending frames.
 * \sa mac_engine_delayed_rx_downlink_frame() */
static void mac_engine_delayed_tx_pending_frame(void)
{
    haddock_assert(mac_info.mac_engine_states == DE_MAC_STATES_JOINED
                   && mac_info.joined_states == DE_JOINED_STATES_IDLE);

    if (! mac_tx_frames_has_pending_tx())
        return;

    static os_uint32 bcn_span;
    static os_uint32 downlink_span;
    
    static struct time bcn_to_now;
    static os_uint32 bcn_to_now_ms;

    static os_uint32 delay_ms;

    downlink_span = gl_bcn_info->beacon_section_length_us *
                    gl_bcn_info->ratio.slots_downlink_msg / 1000;
    bcn_span = gl_bcn_info->beacon_section_length_us *
               gl_bcn_info->ratio.slots_beacon / 1000;

    haddock_time_calc_delta_till_now(& gl_bcn_time->tx, & bcn_to_now);
    bcn_to_now_ms = (bcn_to_now.s * 1000) + bcn_to_now.ms;
    bcn_to_now_ms = (bcn_to_now_ms > bcn_span) ? bcn_span : bcn_to_now_ms;

    delay_ms = (bcn_span - bcn_to_now_ms) + downlink_span;

    mac_joined_state_transfer(DE_JOINED_STATES_TX_FRAME);

    if (delay_ms == 0) {
        os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_SEND_FRAME);
    } else {
        os_timer_reconfig(gl_timeout_timer, this->_pid,
                          SIGNAL_MAC_ENGINE_SEND_FRAME, delay_ms);
        os_timer_start(gl_timeout_timer);

        if (delay_ms >= LPWAN_DE_SLEEP_MIN_NEXT_TIMER_LENGTH_MS) {
            process_sleep();
        }
    }
}

/**
 * Tx message @fbuf during the beacon period's uplink sections.
 */
static void mac_engine_do_tx_msg(const struct tx_fbuf *fbuf)
{
    static os_uint32 uplink_span;       /**< total uplink slots' time */
    static os_uint32 tx_min_span;       /**< the minimum time that should be reserved for tx */

    static os_uint32 try_tx_duration;   /**< try to tx -
                                             1) delay rand(1, try_tx_duration) ms;
                                             2) then perform CCA and tx. */

    mac_tx_frames_prepare_tx_msg(
            fbuf, gl_bcn_info->beacon_seq_id,
            calc_expected_beacon_seq_id(gl_bcn_info->beacon_seq_id, gl_bcn_info->packed_ack_delay_num),
            gl_bcn_info->beacon_class_seq_id,
            gl_bcn_signal_strength->rssi, gl_bcn_signal_strength->snr);

    uplink_span = (gl_bcn_info->beacon_section_length_us * gl_bcn_info->ratio.slots_uplink_msg) / 1000;
    tx_min_span = (gl_bcn_info->beacon_section_length_us * LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS) / 1000;

    try_tx_duration = uplink_span - tx_min_span;

    switch_rlc_caller();
    radio_controller_tx(fbuf->frame, fbuf->len, try_tx_duration);

    if (try_tx_duration >= LPWAN_DE_SLEEP_MIN_NEXT_TIMER_LENGTH_MS) {
        process_sleep();
    }
}

/**
 * \ref radio link controller's tx fail signals: \file rlc_callback_signals.h
 */
static void mac_engine_handle_tx_msg_fail(const struct tx_fbuf *fbuf, signal_bv_t signal)
{
    haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINED);

    if (signal & SIGNAL_RLC_TX_CCA_FAILED) {
        print_log(LOG_WARNING, "rlc_tx CCA fail >%d", fbuf->seq);
    }
    else if (signal & SIGNAL_RLC_TX_CCA_CRC_FAIL) {
        print_log(LOG_WARNING, "JD: rlc_tx CCA CRC fail >%d", fbuf->seq);
    }
    else if (signal & SIGNAL_RLC_TX_TIMEOUT) {
        print_log(LOG_WARNING, "JD: rlc_tx timeout >%d", fbuf->seq);
    } else {
        __should_never_fall_here();
    }
    mac_tx_frames_handle_tx_msg_failed(fbuf);
    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
    process_sleep();
}

static void mac_engine_handle_tx_msg_ok(const struct tx_fbuf *fbuf)
{
    mac_tx_frames_handle_tx_msg_ok(fbuf);

    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
    process_sleep();
}

/**
 * Delay @delay_ms to search beacon again.
 */
static void mac_engine_delayed_start_joining(os_uint32 delay_ms)
{
    haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_INITED);

    os_timer_reconfig(gl_timeout_timer, this->_pid,
                      SIGNAL_MAC_ENGINE_START_JOINING, delay_ms);
    os_timer_start(gl_timeout_timer);

    if (delay_ms > LPWAN_DE_SLEEP_MIN_NEXT_TIMER_LENGTH_MS) {
        process_sleep();
    }
}

#define DE_MAC_ENGINE_IS_VIRTUAL_JOIN

/**
 * Init the JOIN_REQ.
 * \sa DE_MAC_ENGINE_IS_VIRTUAL_JOIN
 */
static void mac_engine_init_join_req(void)
{
#ifdef LPWAN_DEBUG_ONLY_TRACK_BEACON
#define DE_MAC_ENGINE_IS_VIRTUAL_JOIN   // if we only need to track beacon, do virtual JOIN_REQ
#endif

#ifdef DE_MAC_ENGINE_IS_VIRTUAL_JOIN
    // app id, uuid, put tx buffer mgmt join req
    mac_info.short_addr = (os_uint16) mac_info.suuid;
    mac_tx_frames_mgmt_handle_join_ok();

    mac_state_transfer(DE_MAC_STATES_JOINED);
    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);

    print_log(LOG_INFO, "JI: => joined (%04x)", (os_uint16) mac_info.short_addr);
    process_sleep();
#else
    // perform real JOIN_REQ
    // todo
#endif
}

static void device_mac_get_uuid(modem_uuid_t *uuid)
{
    mcu_read_unique_id(uuid);

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
    os_int32 seed = mcu_generate_seed_from_uuid(& mac_info.uuid);
    hdk_srand(seed);
}

/**
 * Send message (emergent/event/routine) after the end-device(s) have joined
 * the network.
 *
 * \return 0 if ok, negative value if failed.
 *
 * \sa DEVICE_SEND_MSG_ERR_NOT_JOINED
 * \sa DEVICE_SEND_MSG_ERR_INVALID_LEN
 * \sa DEVICE_SEND_MSG_ERR_FRAME_BUFFER_FULL
 */
os_int8 device_mac_send_msg(enum device_message_type type, const os_uint8 msg[], os_uint8 len)
{
    if (mac_info.mac_engine_states != DE_MAC_STATES_JOINED)
        return DEVICE_SEND_MSG_ERR_NOT_JOINED;

    if (len > LPWAN_MAX_PAYLAOD_LEN)
        return DEVICE_SEND_MSG_ERR_INVALID_LEN;

    os_int8 flength = \
        mac_tx_frames_put_msg(type, msg, len,
                              mac_info.short_addr, mac_info.gateway_cluster_addr);

    if (flength == -1) {
        print_log(LOG_WARNING, "JD: tx_put (alloc fail)");
        return DEVICE_SEND_MSG_ERR_FRAME_BUFFER_FULL;
    }
    
    print_log(LOG_INFO, "JD: tx_put (%s F%dB P%dB)",
              device_msg_type_string[type], flength, len);

    return 0;
}

enum device_mac_states mac_info_get_mac_states(void)
{
    return mac_info.mac_engine_states;
}

/**
 * \remark make sure to call device_mac_get_uuid() first.
 */
void mac_info_get_uuid(modem_uuid_t *uuid)
{
    *uuid = mac_info.uuid;
}

/**
 * \remark make sure to call device_mac_get_uuid() and device_mac_calculate_suuid() first.
 */
void mac_info_get_suuid(short_modem_uuid_t *suuid)
{
    *suuid = mac_info.suuid;
}

void mac_info_get_short_addr(short_addr_t *short_addr)
{
    haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINED);
    *short_addr = mac_info.short_addr;
}

/**
 * If end-device's SUUID % beacon's class_seq_id == 0, the end-device is allowed
 * to perform tx during the uplink contention field.
 */
os_boolean mac_engine_is_allow_tx(os_uint8 class_seq_id)
{
    if (((os_uint16) mac_info.suuid) % class_seq_id == 0)
        return OS_TRUE;
    else
        return OS_FALSE;
}

