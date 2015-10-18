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
#include "protocol_utils/parse_gw_join_response.h"

#include "beacon_tracker/beacon_tracker.h"
#include "tx_frames_mgmt/tx_frames_mgmt.h"

#include "app_config.h"
#include "mac_engine.h"

/*---------------------------------------------------------------------------*/
/**< MAC information base etc. @{ */
static struct lpwan_device_mac_info mac_info __attribute__((aligned (4)));

/** alias(es) */
static const struct parsed_beacon_info *gl_bcn_info;
static const struct parsed_beacon_packed_ack_to_me *gl_bcn_ack;
static const struct beacon_tracker_info_base *gl_btracker_info_base;
static const struct lpwan_last_rx_frame_rssi_snr *gl_bcn_signal_strength;

/**< @} */
/*---------------------------------------------------------------------------*/
/**< static function declarations @{ */

static signal_bv_t device_mac_engine_entry(os_pid_t pid, signal_bv_t signal);
static inline void mac_state_transfer(enum device_mac_states state);
static inline void mac_joining_state_transfer(enum device_mac_joining_states state);
static inline void mac_joined_state_transfer(enum device_mac_joined_states state);

static os_int8 calc_expected_beacon_seq_id(os_int8 cur_seq_id,
                                           os_int8 packed_ack_delay_num);

static os_int8 mac_engine_is_valid_gw_join_confirm(const os_uint8 *rx_buf, os_uint8 buf_len,
                                                   short_addr_t expected_gw_addr,
                                                   struct parsed_gw_join_confirmed *confirmed);

static void mac_engine_handle_join_confirmed(enum gw_join_confirmed_info info,
                                             short_addr_t distributed_short_addr);
static void mac_engine_handle_join_denied(enum gw_join_confirmed_info info);

static void mac_engine_delayed_rx_downlink_frame(void);
static void mac_engine_delayed_tx_pending_frame(void);
static void mac_engine_do_rx_frame(os_uint16 duration);
static void mac_engine_do_tx_frame(const struct tx_fbuf *fbuf);

static signal_t mac_engine_handle_tx_frame_fail(const struct tx_fbuf *fbuf, signal_bv_t signal);
static void mac_engine_handle_tx_frame_ok(const struct tx_fbuf *fbuf);

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

    mac_info.app_id = (app_id_t) cfg_GetAppID();
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
            mac_joining_state_transfer(DE_JOINING_STATES_SEARCH_BEACON);

            os_ipc_set_signal(this->_pid, SIGNAL_MAC_ENGINE_SEARCH_BEACON);
            return signal ^ SIGNAL_MAC_ENGINE_START_JOINING;
        }

        __rx_wrong_signal(signal);
        break;
    case DE_MAC_STATES_JOINING:
        switch ((int) mac_info.joining_states) {
        case DE_JOINING_STATES_SEARCH_BEACON:
            if (signal & SIGNAL_MAC_ENGINE_SEARCH_BEACON) {
                // beginning of the joining process (search beacon, send join request ...)
                mac_joining_state_transfer(DE_JOINING_STATES_SEARCH_BEACON);

                btracker_search_beacon(DEVICE_JOINING_SEARCH_BCN_TIMEOUT_MS);

                print_log(LOG_INFO, "JI: @{ (%lds:%ldms)",
                          ((os_uint32) DEVICE_JOINING_SEARCH_BCN_TIMEOUT_MS) / 1000,
                          ((os_uint32) DEVICE_JOINING_SEARCH_BCN_TIMEOUT_MS) % 1000);

                process_sleep();
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
                    // Valid beacon found which allows for joining. Get ready to tx JOIN_REQ.
                    os_ipc_set_signal(gl_btracker_pid, SIGNAL_BTRACKER_BEGIN_BEACON_TRACKING);

                    mac_joining_state_transfer(DE_JOINING_STATES_BEACON_FOUND);
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
            break;
        case DE_JOINING_STATES_BEACON_FOUND:
            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACKED) {
                /* Beacon tracked, check if is waiting for JOIN_CONFIRM,
                 * else check if has JOIN_REQ. */
                if (mac_tx_frames_has_waiting_join_confirm(gl_bcn_info->beacon_seq_id)) {
                    if (gl_bcn_ack->has_ack && gl_bcn_ack->is_msg_pending) {
                        if (mac_tx_frames_handle_join_ack(gl_bcn_info->beacon_seq_id,
                                                          gl_bcn_ack->confirmed_seq)) {
                            // get ready to receive downlink JOIN_CONFIRM frame
                            mac_engine_delayed_rx_downlink_frame();

                            print_log(LOG_INFO_COOL, "JI: tracked (%d) join_req ACK(%d)",
                                      gl_bcn_info->beacon_seq_id,
                                      gl_bcn_ack->confirmed_seq);
                        } else {
                            /**
                             * \note suuid conflict, discard it.
                             * \ref mac_tx_frames_handle_join_ack() */

                            print_log(LOG_WARNING, "JI: tracked (%d) possible suuid conflict",
                                      gl_bcn_info->beacon_seq_id);
                        }
                    } else {
                        // no valid JOIN_CONFIRM ACK received
                        mac_tx_frames_handle_no_join_ack(gl_bcn_info->beacon_seq_id);

                        print_log(LOG_WARNING, "JI: tracked (%d) no join_req ACK",
                                  gl_bcn_info->beacon_seq_id);
                    }
                } else if (mac_tx_frames_has_pending_join_req()
                           && gl_bcn_info->ratio.slots_uplink_msg >= DE_MAC_ENGINE_MIN_UPLINK_SLOTS) {
                    // has pending JOIN_REQ, try to tx JOIN_REQ.
                    mac_engine_delayed_tx_pending_frame();

                    print_log(LOG_INFO, "JI: tracked (%d) try tx join_req",
                              gl_bcn_info->beacon_seq_id);
                }

                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACKED;
            }

            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON) {
                mac_tx_frames_handle_lost_beacon();
                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON;
            }

            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED) {
                mac_tx_frames_handle_lost_beacon();

                mac_state_transfer(DE_MAC_STATES_INITED);
                mac_engine_delayed_start_joining(LPWAN_MAC_SEARCH_BCN_AGAIN_MS);

                print_log(LOG_WARNING, "JI: track failed. delayJI %lds:%dms",
                          LPWAN_MAC_SEARCH_BCN_AGAIN_MS / 1000, LPWAN_MAC_SEARCH_BCN_AGAIN_MS % 1000);

                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED;
            }
            break;
        case DE_JOINING_STATES_TX_JOIN_REQ:
            if (signal & SIGNAL_MAC_ENGINE_SEND_FRAME) {
                // assert that there is no awaiting JOIN_CONFIRM and pending JOIN_REQ
                haddock_assert(mac_tx_frames_has_pending_join_req()
                               && !mac_tx_frames_has_waiting_join_confirm(gl_bcn_info->beacon_seq_id));

                cur_tx.tx_frame = mac_tx_frames_get_join_frame();
                mac_engine_do_tx_frame(cur_tx.tx_frame);

                print_log(LOG_INFO, "JI: rlc_tx join_req >%d",
                          cur_tx.tx_frame->seq);

                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
            }
            else if (signal & SIGNAL_RLC_TX_OK) {
                mac_engine_handle_tx_frame_ok(cur_tx.tx_frame);
                process_sleep();

                print_log(LOG_INFO, "JI: rlc_tx ok >%d",
                          cur_tx.tx_frame->seq);
                return signal ^ SIGNAL_RLC_TX_OK;
            }
            else if (signal & SIGNALS_RLC_TX_FAILED) {
                signal_t processed_signal = \
                        mac_engine_handle_tx_frame_fail(cur_tx.tx_frame, signal);
                process_sleep();
                return signal ^ processed_signal;
            }
            __rx_wrong_signal(signal);
            break;
        case DE_JOINING_STATES_RX_JOIN_CONFIRM:
            if (signal & SIGNAL_MAC_ENGINE_RECV_FRAME) {
                // start to try to receive downlink frame
                mac_engine_do_rx_frame(DEVICE_MAC_RECV_DOWNLINK_TIMEOUT_MS);
                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_RECV_FRAME;
            }

            if (signal & SIGNAL_RLC_RX_OK) {
                static struct parsed_gw_join_confirmed confirmed;
                os_int8 len = mac_engine_is_valid_gw_join_confirm(gl_radio_rx_buffer,
                                                       1+LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                                                       gl_bcn_info->gateway_cluster_addr,
                                                       & confirmed);
                if (len > 0) {
                    // valid JOIN_CONFIRM from gateway
                    radio_controller_rx_stop();
                    os_timer_stop(gl_timeout_timer);

                    switch (confirmed.info) {
                    case JOIN_CONFIRMED_PAID:
                    case JOIN_CONFIRMED_NOT_PAID:
                        mac_engine_handle_join_confirmed(confirmed.info,
                                                         confirmed.distributed_short_addr);
                        break;
                    case JOIN_DENIED_DEFAULT:
                    case JOIN_DENIED_LOCATION_ERR:
                        // we don't handle them seperately currently.
                        mac_engine_handle_join_denied(confirmed.info);
                        break;
                    default:
                        __should_never_fall_here();
                        break;
                    }
                } else {
                    print_log(LOG_INFO, "JI: rx not join_confirm");
                }

                process_sleep();
                return signal ^ SIGNAL_RLC_RX_OK;
            }

            if (signal & SIGNAL_RLC_RX_DURATION_TIMEOUT) {
                os_timer_stop(gl_timeout_timer);
                print_log(LOG_WARNING, "JI: rlc_rx timeout");

                mac_joining_state_transfer(DE_JOINING_STATES_BEACON_FOUND);
                process_sleep();
                return signal ^ SIGNAL_RLC_RX_DURATION_TIMEOUT;
            }

            if (signal & SIGNAL_MAC_ENGINE_RECV_TIMEOUT) {
                radio_controller_rx_stop();
                print_log(LOG_WARNING, "JI: rlc_rx timeout");

                mac_joining_state_transfer(DE_JOINING_STATES_BEACON_FOUND);
                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_RECV_TIMEOUT;
            }

            __rx_wrong_signal(signal);
            break;
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

                print_log(LOG_INFO, "JD: tracked (%d)", gl_bcn_info->beacon_seq_id);
                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACKED;
            }

            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON) {
                mac_tx_frames_handle_lost_beacon();
                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON;
            }

            if (signal & SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED) {
                mac_tx_frames_handle_lost_beacon();

                mac_state_transfer(DE_MAC_STATES_INITED);
                mac_engine_delayed_start_joining(LPWAN_MAC_SEARCH_BCN_AGAIN_MS);

                print_log(LOG_WARNING, "JI: @} delayJI %lds:%dms",
                          LPWAN_MAC_SEARCH_BCN_AGAIN_MS / 1000, LPWAN_MAC_SEARCH_BCN_AGAIN_MS % 1000);

                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED;
            }

            __rx_wrong_signal(signal);
            break;
        case DE_JOINED_STATES_RX_FRAME:
            if (signal & SIGNAL_MAC_ENGINE_RECV_FRAME) {
                mac_engine_do_rx_frame(DEVICE_MAC_RECV_DOWNLINK_TIMEOUT_MS);
                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_RECV_FRAME;
            }

            if (signal & SIGNAL_RLC_RX_OK) {
                // todo

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                process_sleep();
                return signal ^ SIGNAL_RLC_RX_OK;
            }

            if (signal & SIGNAL_RLC_RX_DURATION_TIMEOUT) {
                os_timer_stop(gl_timeout_timer);
                print_log(LOG_WARNING, "JD: rlc_rx timeout");

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                process_sleep();
                return signal ^ SIGNAL_RLC_RX_DURATION_TIMEOUT;
            }

            if (signal & SIGNAL_MAC_ENGINE_RECV_TIMEOUT) {
                radio_controller_rx_stop();
                print_log(LOG_WARNING, "JD: rlc_rx timeout");

                mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_RECV_TIMEOUT;
            }

            __rx_wrong_signal(signal);
            break;
        case DE_JOINED_STATES_TX_FRAME:
            if (signal & SIGNAL_MAC_ENGINE_SEND_FRAME) {
                if (! mac_tx_frames_has_pending_tx()
                    || (cur_tx.tx_frame = mac_tx_frames_get_msg()) == NULL) {
                    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
                    return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
                }

                mac_engine_do_tx_frame(cur_tx.tx_frame);

                print_log(LOG_INFO, "JD: rlc_tx (%s %dB %dT %dTF)\t>%d",
                          device_msg_type_string[(int) cur_tx.tx_frame->msg_type],
                          cur_tx.tx_frame->len,
                          cur_tx.tx_frame->transmit_times,
                          cur_tx.tx_frame->tx_fail_times,
                          cur_tx.tx_frame->seq);

                process_sleep();
                return signal ^ SIGNAL_MAC_ENGINE_SEND_FRAME;
            }
            else if (signal & SIGNAL_RLC_TX_OK) {
                mac_engine_handle_tx_frame_ok(cur_tx.tx_frame);

                print_log(LOG_INFO, "JD: rlc_tx ok >%d",
                          cur_tx.tx_frame->seq);
                process_sleep();
                return signal ^ SIGNAL_RLC_TX_OK;
            }
            else if (signal & SIGNALS_RLC_TX_FAILED) {
                signal_t processed_signal = \
                        mac_engine_handle_tx_frame_fail(cur_tx.tx_frame, signal);
                process_sleep();
                return signal ^ processed_signal;
            }
            __rx_wrong_signal(signal);
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

static os_int8 calc_expected_beacon_seq_id(os_int8 cur_seq_id,
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
    haddock_assert((mac_info.mac_engine_states == DE_MAC_STATES_JOINED
                    && mac_info.joined_states == DE_JOINED_STATES_IDLE)
                   ||
                   (mac_info.mac_engine_states == DE_MAC_STATES_JOINING
                    && mac_info.joining_states == DE_JOINING_STATES_BEACON_FOUND));

    if (! (gl_bcn_ack->has_ack && gl_bcn_ack->is_msg_pending))
        return;

    switch (mac_info.mac_engine_states) {
    case DE_MAC_STATES_JOINED:
        mac_joined_state_transfer(DE_JOINED_STATES_RX_FRAME);
        break;
    case DE_MAC_STATES_JOINING:
        mac_joining_state_transfer(DE_JOINING_STATES_RX_JOIN_CONFIRM);
        break;
    default:
        __should_never_fall_here();
    }

    static os_uint32 bcn_remains_ms;
    static os_uint32 prev_downlink_span;

    static os_uint32 delay_ms;

    bcn_remains_ms = btracker_calc_beacon_span_remains();
    prev_downlink_span = gl_bcn_info->beacon_section_length_us *
                         gl_bcn_ack->total_prev_downlink_slots / 1000;

    delay_ms = bcn_remains_ms + prev_downlink_span;
    delay_ms = (delay_ms > DEVICE_MAC_RECV_DOWNLINK_IN_ADVANCE_MS) ?
               (delay_ms - DEVICE_MAC_RECV_DOWNLINK_IN_ADVANCE_MS) : 0;

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

/** \note Only if there is no downlink frame, try to tx pending frames.
 * \sa mac_engine_delayed_rx_downlink_frame() */
static void mac_engine_delayed_tx_pending_frame(void)
{
    haddock_assert((mac_info.mac_engine_states == DE_MAC_STATES_JOINED
                    && mac_info.joined_states == DE_JOINED_STATES_IDLE)
                   ||
                   (mac_info.mac_engine_states == DE_MAC_STATES_JOINING
                    && mac_info.joining_states == DE_JOINING_STATES_BEACON_FOUND));

    if (! mac_tx_frames_has_pending_tx())
        return;

    switch (mac_info.mac_engine_states) {
    case DE_MAC_STATES_JOINED:
        mac_joined_state_transfer(DE_JOINED_STATES_TX_FRAME);
        break;
    case DE_MAC_STATES_JOINING:
        mac_joining_state_transfer(DE_JOINING_STATES_TX_JOIN_REQ);
        break;
    default:
        __should_never_fall_here();
    }

    static os_uint32 bcn_remains_ms;
    static os_uint32 downlink_span;

    static os_uint32 delay_ms;

    bcn_remains_ms = btracker_calc_beacon_span_remains();
    downlink_span = gl_bcn_info->beacon_section_length_us *
                    gl_bcn_info->ratio.slots_downlink_msg / 1000;

    delay_ms = bcn_remains_ms + downlink_span;

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
 * Try to perform rx (downlink JOIN_CONFIRM / message) during @duration ms.
 */
static void mac_engine_do_rx_frame(os_uint16 duration)
{
    os_timer_reconfig(gl_timeout_timer, this->_pid,
                      SIGNAL_MAC_ENGINE_RECV_TIMEOUT, duration);
    os_timer_start(gl_timeout_timer);

    switch_rlc_caller();
    // a little longer duration to avoid race condition.
    radio_controller_rx_continuously(duration + 50);
}

/**
 * Tx message @fbuf during the beacon period's uplink sections.
 */
static void mac_engine_do_tx_frame(const struct tx_fbuf *fbuf)
{
    static os_uint32 uplink_span;       /**< total uplink slots' time */
    static os_uint32 tx_min_span;       /**< the minimum time that should be reserved for tx */

    static os_uint32 try_tx_duration;   /**< try to tx -
                                             1) delay rand(1, try_tx_duration) ms;
                                             2) then perform CCA and tx. */

    mac_tx_frames_prepare_frame_func_t prepare_frame_func = NULL;
    switch (fbuf->ftype) {
    case FTYPE_DEVICE_JOIN:
        prepare_frame_func = mac_tx_frames_prepare_join_req;
        break;
    case FTYPE_DEVICE_MSG:
        prepare_frame_func = mac_tx_frames_prepare_tx_msg;
        break;
    default:
        __should_never_fall_here();
    }

    prepare_frame_func(fbuf, gl_bcn_info->beacon_seq_id,
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
static signal_t mac_engine_handle_tx_frame_fail(const struct tx_fbuf *fbuf, signal_bv_t signal)
{
    signal_t processed_signal;
    if (signal & SIGNAL_RLC_TX_CCA_FAILED) {
        print_log(LOG_WARNING, "rlc_tx CCA fail >%d", fbuf->seq);
        processed_signal = SIGNAL_RLC_TX_CCA_FAILED;
    }
    else if (signal & SIGNAL_RLC_TX_CCA_CRC_FAIL) {
        print_log(LOG_WARNING, "rlc_tx CCA CRC fail >%d", fbuf->seq);
        processed_signal = SIGNAL_RLC_TX_CCA_CRC_FAIL;
    }
    else if (signal & SIGNAL_RLC_TX_TIMEOUT) {
        print_log(LOG_WARNING, "rlc_tx timeout >%d", fbuf->seq);
        processed_signal = SIGNAL_RLC_TX_TIMEOUT;
    } else {
        __rx_wrong_signal(signal);
    }

    switch (fbuf->ftype) {
    case FTYPE_DEVICE_MSG:
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINED
                       && mac_info.joined_states == DE_JOINED_STATES_TX_FRAME);
        mac_tx_frames_handle_tx_msg_failed(fbuf);
        mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
        break;
    case FTYPE_DEVICE_JOIN:
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINING
                       && mac_info.joining_states == DE_JOINING_STATES_TX_JOIN_REQ);
        mac_tx_frames_handle_tx_join_failed();
        mac_joining_state_transfer(DE_JOINING_STATES_BEACON_FOUND);
        break;
    default:
        __should_never_fall_here();
    }
    process_sleep();
    return processed_signal;
}

static void mac_engine_handle_tx_frame_ok(const struct tx_fbuf *fbuf)
{
    switch (fbuf->ftype) {
    case FTYPE_DEVICE_MSG:
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINED
                       && mac_info.joined_states == DE_JOINED_STATES_TX_FRAME);
        mac_tx_frames_handle_tx_msg_ok(fbuf);
        mac_joined_state_transfer(DE_JOINED_STATES_IDLE);
        break;
    case FTYPE_DEVICE_JOIN:
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINING
                       && mac_info.joining_states == DE_JOINING_STATES_TX_JOIN_REQ);
        mac_tx_frames_handle_tx_join_ok();
        mac_joining_state_transfer(DE_JOINING_STATES_BEACON_FOUND);
        break;
    default:
        __should_never_fall_here();
    }

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

/**
 * Init the JOIN_REQ.
 * \sa DE_MAC_ENGINE_IS_VIRTUAL_JOIN
 */
static void mac_engine_init_join_req(void)
{
    haddock_assert(mac_info.mac_engine_states == DE_MAC_STATES_JOINING
                   && mac_info.joining_states == DE_JOINING_STATES_BEACON_FOUND);

#ifdef LPWAN_DEBUG_ONLY_TRACK_BEACON
// if we only need to track beacon, do virtual JOIN_REQ
#define DE_MAC_ENGINE_IS_VIRTUAL_JOIN   OS_TRUE
#endif

#if defined (DE_MAC_ENGINE_IS_VIRTUAL_JOIN) && DE_MAC_ENGINE_IS_VIRTUAL_JOIN == OS_TRUE
    /**
     * We assume JOIN ok.
     * Automatically set the distributed short address as shortened UUID. */
    mac_info.gateway_cluster_addr = gl_bcn_info->gateway_cluster_addr;
    mac_info.short_addr = (os_uint16) mac_info.suuid;

    mac_tx_frames_handle_join_ok(mac_info.short_addr, mac_info.gateway_cluster_addr);

    mac_state_transfer(DE_MAC_STATES_JOINED);
    mac_joined_state_transfer(DE_JOINED_STATES_IDLE);

    print_log(LOG_INFO, "JI: => virtual joined (%04X)", (os_uint16) mac_info.short_addr);
    process_sleep();
#else
    /** perform the real JOIN_REQ */
    mac_tx_frames_init_join_req(gl_bcn_info->gateway_cluster_addr);
#endif
}

/**
 * Check if received valid JOIN_CONFIRM frame from specified gateway @expected_gw_addr.
 */
static os_int8 mac_engine_is_valid_gw_join_confirm(const os_uint8 *rx_buf, os_uint8 buf_len,
                                                   short_addr_t expected_gw_addr,
                                                   struct parsed_gw_join_confirmed *confirmed)
{
    os_int8 len = -1;
    static struct parsed_frame_hdr_info frame_hdr;

    if (rx_buf[0] > 0) {
        len = lpwan_parse_frame_header((void *) (rx_buf+1), rx_buf[0],
                                       & frame_hdr);

        if (len > 0
            && frame_hdr.frame_origin_type == DEVICE_GATEWAY
            && frame_hdr.info.gw.frame_type == FTYPE_GW_JOIN_CONFIRMED)
        {
            if (frame_hdr.src.addr.short_addr != expected_gw_addr) {
                // check if the gw_cluster_addr is matched.
                len = -1;
                goto mac_engine_is_valid_gw_join_confirm_return;
            }

            haddock_assert(0 < len && len <= rx_buf[0]);
            len = lpwan_parse_gw_join_confirmed(rx_buf+1+len, rx_buf[0]-len, confirmed);
        } else {
            len = -1;
        }
    }
mac_engine_is_valid_gw_join_confirm_return:
    return len;
}

static void mac_engine_handle_join_confirmed(enum gw_join_confirmed_info info,
                                             short_addr_t distributed_short_addr)
{
    switch (info) {
    case JOIN_CONFIRMED_NOT_PAID:
        // #mark# handle with the out-of-fee situation.
        // no break, regard as PAID currently.
    case JOIN_CONFIRMED_PAID:
        mac_tx_frames_handle_join_ok(distributed_short_addr,
                                     gl_bcn_info->gateway_cluster_addr);
        mac_state_transfer(DE_MAC_STATES_JOINED);
        mac_joined_state_transfer(DE_JOINED_STATES_IDLE);

        mac_info.short_addr = distributed_short_addr;
        mac_info.gateway_cluster_addr = gl_bcn_info->gateway_cluster_addr;

        print_log(LOG_INFO_COOL, "JI: => joined (%04X)",
                  (os_uint16) distributed_short_addr);
        break;
    default:
        __should_never_fall_here();
        break;
    }
}

static void mac_engine_handle_join_denied(enum gw_join_confirmed_info info)
{
    switch (info) {
    case JOIN_DENIED_LOCATION_ERR:
        // #mark# handle with the location error situation (gateway blacklist).
        // no break, fall through
    case JOIN_DENIED_DEFAULT:
        mac_tx_frames_handle_join_denied();

        mac_state_transfer(DE_MAC_STATES_INITED);
        mac_engine_delayed_start_joining(LPWAN_MAC_SEARCH_BCN_AGAIN_MS);

        print_log(LOG_WARNING, "JI: join denied.");
        break;
    default:
        __should_never_fall_here();
        break;
    }
}

static void device_mac_get_uuid(modem_uuid_t *uuid)
{
    mcu_read_unique_id(uuid);

    print_log(LOG_INFO,
              "INIT: UUID %02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X",
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
    print_log(LOG_INFO, "INIT: suuid %04X", (os_uint16) mac_info.suuid);
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
short_modem_uuid_t mac_info_get_suuid(void)
{
    return mac_info.suuid;
}

short_addr_t mac_info_get_short_addr(void)
{
    haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINED);
    return mac_info.short_addr;
}

app_id_t mac_info_get_app_id(void)
{
    return mac_info.app_id;
}

/**
 * If end-device's SUUID % beacon's classes_num == beacon's class_seq_id, the end-device is allowed
 * to perform tx during the uplink contention field.
 */
os_boolean mac_engine_is_allow_tx(os_uint8 classes_num, os_uint8 class_seq_id)
{
    if (((os_uint16) mac_info.suuid) % classes_num == class_seq_id)
        return OS_TRUE;
    else
        return OS_FALSE;
}

