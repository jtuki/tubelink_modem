/**
 * beacon_tracker.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "kernel/timer.h"
#include "kernel/ipc.h"
#include "kernel/sys_signal_defs.h"

#include "lib/hdk_utilities.h"
#include "lib/assert.h"

#include "protocol_utils/parse_beacon.h"

#include "end_device/tx_frames_mgmt/tx_frames_mgmt.h"
#include "radio_controller/radio_controller.h"
#include "end_device/mac_engine.h"

#include "lpwan_config.h"
#include "beacon_tracker.h"

os_pid_t gl_btracker_pid;
haddock_process("btracker_proc");

/**
 * The global timeout timer, used by both @btracker_search_timeout_timer
 * and @btracker_track_timeout_timer. At any time, only a single timer _should_
 * use it.
 */
static struct timer *btracker_timeout_timer;
static struct timer *btracker_track_periodically_timer;

static struct timer *btracker_search_timeout_timer;
static struct timer *btracker_track_timeout_timer;


/** Beacon tracker layer's state. */
static enum beacon_tracker_states gl_btracker_states;

/**
 * Rx beacon's information.
 * \sa btracker_get_parsed_frame_hdr_info()
 * \sa btracker_get_parsed_beacon_info()
 * \sa btracker_get_parsed_packed_ack()
 */
static struct parsed_frame_hdr_info             btracker_frame_hdr_info;
static struct parsed_beacon_info                btracker_beacon_info;
static struct parsed_beacon_packed_ack_to_me    btracker_beacon_packed_ack;

static struct {
    struct lpwan_last_rx_frame_rssi_snr signal_strength;
    struct lpwan_last_rx_frame_time time;
} btracker_beacon_meta_info;

/**
 * Last synced beacon's information: frame header, beacon body, packed ACK.
 * \sa btracker_frame_hdr_info
 * \sa btracker_beacon_info
 * \sa btracker_beacon_packed_ack
 */
static struct parsed_frame_hdr_info             gl_synced_fhdr;
static struct parsed_beacon_info                gl_synced_bcn;
static struct parsed_beacon_packed_ack_to_me    gl_synced_ack;

static struct beacon_tracker_info_base btracker_info_base;

/** \sa btracker_register_mac_engine() */
static os_pid_t gl_registered_mac_engine_pid = PROCESS_ID_RESERVED;

/*---------------------------------------------------------------------------*/
/**< Internal functions. @{ */

static os_int8 btracker_is_valid_beacon(const os_uint8 *rx_buf, os_uint8 buf_len,
                                        const short_addr_t *expected_gw_cluster_addr);
static os_boolean btracker_is_match_expected_beacon(void);

static os_boolean btracker_is_virtual_track_beacon(void);
static void btracker_parse_beacon_config(struct parse_beacon_check_info *info);

static void btracker_sync_beacon_frame_info(void);
static void btracker_sync_info_base(void);
static void btracker_info_base_ticktock(void);

static void btracker_handle_beacon_not_found(void);

static void btracker_start_periodical_track_timer(os_uint16 delay_ms);
static void btracker_stop_periodical_track_timer(void);

static os_uint16 btracker_calc_beacon_span(void);
static os_uint16 btracker_calc_beacon_rx_till_now_delta(void);
static os_uint16 btracker_calc_beacon_tx_till_now_delta(void);

static void btracker_do_begin_bcn_tracking(void);
static void btracker_do_bcn_tracking(void);

static void btracker_handle_track_bcn_ok(void);
static void btracker_handle_track_bcn_timeout(void);

static void btracker_do_track_search(void);
static void btracker_handle_track_search_timeout(void);

static signal_bv_t btracker_entry(os_pid_t pid, signal_bv_t signal);
/**< @} */
/*---------------------------------------------------------------------------*/

void btracker_init(os_int8 priority)
{
    struct process *proc_btracker = process_create(btracker_entry, priority);
    haddock_assert(proc_btracker);
    gl_btracker_pid = proc_btracker->_pid;

    btracker_timeout_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                             ((os_uint32)1)<<31);
    btracker_track_periodically_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                             ((os_uint32)1)<<31);

    btracker_search_timeout_timer = NULL;
    btracker_track_timeout_timer = NULL;

    gl_btracker_states = BCN_TRACKER_INITED;
    os_ipc_set_signal(this->_pid, SIGNAL_BTRACKER_INIT_FINISHED);
}

static signal_bv_t btracker_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);

    switch (gl_btracker_states) {
    case BCN_TRACKER_INITED:
        if (signal & SIGNAL_BTRACKER_INIT_FINISHED) {
            haddock_assert(gl_registered_mac_engine_pid != PROCESS_ID_RESERVED);
            return signal ^ SIGNAL_BTRACKER_INIT_FINISHED;
        }
        __should_never_fall_here();
        break;
    case BCN_TRACKER_SEARCH_BEACON:
        if (signal & SIGNAL_BTRACKER_SEARCH_TIMEOUT) {
            radio_controller_rx_stop();
            btracker_handle_beacon_not_found();
            return signal ^ SIGNAL_BTRACKER_SEARCH_TIMEOUT;
        }

        if (signal & SIGNAL_RLC_RX_DURATION_TIMEOUT) {
            os_timer_stop(btracker_search_timeout_timer);
            btracker_handle_beacon_not_found();
            return signal ^ SIGNAL_RLC_RX_DURATION_TIMEOUT;
        }

        if (signal & SIGNAL_RLC_RX_OK) {
            if (signal & SIGNAL_RLC_RX_OK) {
                os_int8 len = btracker_is_valid_beacon(gl_radio_rx_buffer,
                                                       1+LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                                                       NULL);
                if (len > 0) {
                    // valid beacon frame
                    radio_controller_rx_stop();
                    os_timer_stop(btracker_search_timeout_timer);

                    btracker_beacon_meta_info.signal_strength = lpwan_radio_get_last_rx_rssi_snr();
                    lpwan_radio_get_last_rx_frame_time(& btracker_beacon_meta_info.time);

                    os_ipc_set_signal(gl_registered_mac_engine_pid, SIGNAL_MAC_ENGINE_BEACON_FOUND);
                    gl_btracker_states = BCN_TRACKER_BEACON_FOUND;
                } else {
                    print_log(LOG_INFO, "BTR (search): rx not beacon");
                }
                return signal ^ SIGNAL_RLC_RX_OK;
            }
        }
        __should_never_fall_here();
        break;
    case BCN_TRACKER_BEACON_FOUND:
        if (signal & SIGNAL_BTRACKER_BEGIN_BEACON_TRACKING) {
            btracker_do_begin_bcn_tracking();
            gl_btracker_states = BCN_TRACKER_BCN_TRACKING;
            return signal ^ SIGNAL_BTRACKER_BEGIN_BEACON_TRACKING;
        }
        __should_never_fall_here();
        break;
    case BCN_TRACKER_BCN_TRACKING:
        if (signal & SIGNAL_BTRACKER_TRACK_BEACON) {
            btracker_info_base_ticktock(); // perform ticktock to update expected sequence id

            os_boolean is_virtual_tracking = btracker_is_virtual_track_beacon();

            /* If is_virtual_tracking == TRUE, we don't perform real beacon tracking,
             * btracker_info_base_ticktock() is enough.
             */
            if (! is_virtual_tracking) {
                btracker_do_bcn_tracking();
            } else {
                /* As to virtual tracking, simply restart the @btracker_track_periodically_timer */
                os_uint16 delta = gl_synced_bcn.beacon_period_length_s * 1000;
                btracker_start_periodical_track_timer(delta);
            }

            return signal ^ SIGNAL_BTRACKER_TRACK_BEACON;
        }

        if (signal & SIGNAL_RLC_RX_OK) {
            os_int8 len = btracker_is_valid_beacon(gl_radio_rx_buffer,
                                                   1+LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                                                   & gl_synced_bcn.gateway_cluster_addr);
            if (len > 0) {
                static struct {
                    os_int8 seq_id;
                    os_int8 expected_seq_id;
                } seq;

                // track ok! valid beacon frame from the same gateway
                radio_controller_rx_stop();
                os_timer_stop(btracker_track_timeout_timer);

                os_boolean is_match_expected_beacon = btracker_is_match_expected_beacon();
                if (! is_match_expected_beacon) {
                    seq.expected_seq_id = btracker_info_base.expected_beacon_seq_id;
                    seq.seq_id = btracker_beacon_info.beacon_seq_id;
                }

                btracker_handle_track_bcn_ok();
                os_ipc_set_signal(gl_registered_mac_engine_pid, SIGNAL_MAC_ENGINE_BEACON_TRACKED);

                /*
                 * print_log() will invoke IO operation (UART interruption etc.) which will
                 * seemingly block some time, which in thus will cause some delay.
                 * So we put print_log() to the end of the block, to avoid the interference
                 * with the timing of btracker_handle_track_bcn_ok().
                 */
                if (! is_match_expected_beacon) {
                    print_log(LOG_WARNING, "BTR: mis-match (E:%d; R:%d)",
                              seq.expected_seq_id,
                              seq.seq_id);
                }
            } else {
                print_log(LOG_INFO, "BTR (track): rx not beacon");
            }

            return signal ^ SIGNAL_RLC_RX_OK;
        }

        if (signal & SIGNAL_RLC_RX_DURATION_TIMEOUT) {
            os_timer_stop(btracker_track_timeout_timer);
            btracker_handle_track_bcn_timeout();

            gl_btracker_states = BCN_TRACKER_BCN_TRACKING_SEARCH;
            return signal ^ SIGNAL_RLC_RX_DURATION_TIMEOUT;
        }

        if (signal & SIGNAL_BTRACKER_TRACK_TIMEOUT) {
            radio_controller_rx_stop();
            btracker_handle_track_bcn_timeout();

            gl_btracker_states = BCN_TRACKER_BCN_TRACKING_SEARCH;
            return signal ^ SIGNAL_BTRACKER_TRACK_TIMEOUT;
        }

        __should_never_fall_here();
        break;
    case BCN_TRACKER_BCN_TRACKING_SEARCH:
        if (signal & SIGNAL_BTRACKER_BEGIN_TRACK_SEARCH) {
            btracker_do_track_search();
            return signal ^ SIGNAL_BTRACKER_BEGIN_TRACK_SEARCH;
        }

        if (signal & SIGNAL_RLC_RX_OK) {
            os_int8 len = btracker_is_valid_beacon(gl_radio_rx_buffer,
                                                   1+LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                                                   & gl_synced_bcn.gateway_cluster_addr);
            if (len > 0) {
                // track ok! valid beacon frame from the same gateway
                radio_controller_rx_stop();
                os_timer_stop(btracker_search_timeout_timer);

                btracker_handle_track_bcn_ok();
                gl_btracker_states = BCN_TRACKER_BCN_TRACKING;
                os_ipc_set_signal(gl_registered_mac_engine_pid, SIGNAL_MAC_ENGINE_BEACON_TRACKED);
            } else {
                print_log(LOG_INFO, "BTR (track-search): rx not beacon");
            }
            return signal ^ SIGNAL_RLC_RX_OK;
        }

        if (signal & SIGNAL_RLC_RX_DURATION_TIMEOUT) {
            os_timer_stop(btracker_search_timeout_timer);
            btracker_handle_track_search_timeout();
            return signal ^ SIGNAL_RLC_RX_DURATION_TIMEOUT;
        }

        if (signal & SIGNAL_BTRACKER_TRACK_SEARCH_TIMEOUT) {
            radio_controller_rx_stop();
            btracker_handle_track_search_timeout();
            return signal ^ SIGNAL_BTRACKER_TRACK_SEARCH_TIMEOUT;
        }

        __should_never_fall_here();
        break;
    default:
        break;
    }

    // discard unknown signal.
    return 0;
}

/**
 * Reset beacon tracker layer to BCN_TRACKER_INITED states.
 *
 * \note The radio link controller layer will also be reset.
 * \sa rlc_reset()
 */
void btracker_reset(void)
{
    // also reset the radio link controller.
    rlc_reset();

    os_timer_stop(btracker_timeout_timer);
    btracker_stop_periodical_track_timer();

    btracker_search_timeout_timer = NULL;
    btracker_track_timeout_timer = NULL;

    gl_btracker_states = BCN_TRACKER_INITED;
}

void btracker_register_mac_engine(os_pid_t mac_engine_pid)
{
    gl_registered_mac_engine_pid = mac_engine_pid;
}

/**
 * Search beacon for @duration (ms).
 *
 * \note @gl_btracker_states should be BCN_TRACKER_INITED state.
 * \sa btracker_reset()
 */
void btracker_search_beacon(os_uint16 duration)
{
    haddock_assert(duration < 60*1000); // should be less than 60 seconds
    haddock_assert(gl_btracker_states == BCN_TRACKER_INITED);

    btracker_search_timeout_timer = btracker_timeout_timer;
    os_timer_reconfig(btracker_search_timeout_timer,
                      this->_pid,
                      SIGNAL_BTRACKER_SEARCH_TIMEOUT, duration);
    os_timer_start(btracker_search_timeout_timer);

    switch_rlc_caller();
    radio_controller_rx_stop(); // explicitly put radio to IDLE mode.
    /** A little longer to avoid race condition. */
    radio_controller_rx_continuously(duration + 10);

    gl_btracker_states = BCN_TRACKER_SEARCH_BEACON;
}

/**
 * Config parse beacon's behavior (store the configuration into @info).
 * \note During BCN_TRACKER_BCN_TRACKING state, make sure btracker_is_virtual_track_beacon()
 *       has been performed before btracker_parse_beacon_config().
 */
static void btracker_parse_beacon_config(struct parse_beacon_check_info *info)
{
    static os_boolean run_first_time = OS_TRUE;
    if (run_first_time) {
        mac_info_get_uuid(& info->uuid);
        mac_info_get_suuid(& info->suuid);
        run_first_time = OS_FALSE;
    }

    if (gl_btracker_states == BCN_TRACKER_SEARCH_BEACON) {
        // MAC layer just tried to join, no JOIN_REQ sent currently.
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINING);
        info->is_check_packed_ack = OS_FALSE;
        info->is_check_join_ack = OS_FALSE;

        return;
    }

    if (gl_btracker_states == BCN_TRACKER_BCN_TRACKING) {
        /** \sa btracker_is_virtual_track_beacon() */
        info->is_check_packed_ack = btracker_info_base.is_check_packed_ack;
        info->is_check_join_ack = btracker_info_base.is_check_join_ack;

        if (mac_info_get_mac_states() == DE_MAC_STATES_JOINED) {
            mac_info_get_short_addr(& info->short_addr);
        }
        return;
    }

    if (gl_btracker_states == BCN_TRACKER_BCN_TRACKING_SEARCH) {
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINED);
        // we don't care about the packed ACK during miss tracking condition.
        info->is_check_packed_ack = OS_FALSE;
        info->is_check_join_ack = OS_FALSE;
        return;
    }

    __should_never_fall_here();
}

/**
 * @rx_buf is the RLC layer's rx buffer.
 * @len is the buffer's length.
 * \return (len(beacon_frame) - len(frame_header)) if valid, -1 otherwise.
 *
 * \note Parsed information is stored in:
 *      btracker_frame_hdr_info, btracker_beacon_info, btracker_beacon_packed_ack.
 *
 * \sa gl_radio_rx_buffer
 */
static os_int8 btracker_is_valid_beacon(const os_uint8 *rx_buf, os_uint8 buf_len,
                                        const short_addr_t *expected_gw_cluster_addr)
{
    static struct parse_beacon_check_info check_info;
    btracker_parse_beacon_config(& check_info);

    os_int8 len = -1;
    if (rx_buf[0] > 0) {
        len = lpwan_parse_frame_header((void *) (rx_buf+1), rx_buf[0],
                                         & btracker_frame_hdr_info);

        if (len > 0
            && btracker_frame_hdr_info.frame_origin_type == DEVICE_GATEWAY
            && btracker_frame_hdr_info.info.gw.frame_type == FTYPE_GW_BEACON)
        {
            if (expected_gw_cluster_addr
                && btracker_frame_hdr_info.src.addr.short_addr != *expected_gw_cluster_addr) {
                // check if the gw_cluster_addr is matched.
                len = -1;
                goto btracker_is_valid_beacon_return;
            }

            haddock_assert(0 < len && len <= rx_buf[0]);
            len = lpwan_parse_beacon(rx_buf+1+len, rx_buf[0]-len,
                                     & check_info,
                                     & btracker_beacon_info,
                                     & btracker_beacon_packed_ack);
            if (expected_gw_cluster_addr == NULL) {
                // if gw_cluster_addr != NULL, no need to copy the gateway_cluster_addr.
                btracker_beacon_info.gateway_cluster_addr = \
                        btracker_frame_hdr_info.src.addr.short_addr;
            }
        } else {
            len = -1;
        }
    }
btracker_is_valid_beacon_return:
    return len;
}

/**
 * \note If ::gateway_cluster_addr and ::beacon_seq_id are expected, we regard it's
 *       a match. We don't care about ::beacon_class_seq_id and ::beacon_classes_num.
 * \note As to ::gateway_cluster_addr, \sa btracker_is_valid_beacon().
 */
static os_boolean btracker_is_match_expected_beacon(void)
{
    if (btracker_beacon_info.gateway_cluster_addr == gl_synced_bcn.gateway_cluster_addr
        && btracker_beacon_info.beacon_seq_id == btracker_info_base.expected_beacon_seq_id)
        return OS_TRUE;
    else
        return OS_FALSE;
}

/**
 * Beacon not found during BCN_TRACKER_SEARCH_BEACON state.
 * \sa btracker_handle_track_search_timeout()
 */
static void btracker_handle_beacon_not_found(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_SEARCH_BEACON);
    btracker_reset();
    os_ipc_set_signal(gl_registered_mac_engine_pid, SIGNAL_MAC_ENGINE_BEACON_NOT_FOUND);
}

/**
 * Copy previously tracked beacon frame's information (frame header, beacon body,
 * packed ACK) to corresponding _sync_ objects.
 * \sa gl_synced_fhdr
 * \sa gl_synced_bcn
 * \sa gl_synced_ack
 */
static void btracker_sync_beacon_frame_info(void)
{
    gl_synced_fhdr = btracker_frame_hdr_info;
    gl_synced_bcn = btracker_beacon_info;
    gl_synced_ack = btracker_beacon_packed_ack;
}

/**
 * After a valid beacon is tracked, synchronize the beacon related information in
 * @btracker_info_base.
 */
static void btracker_sync_info_base(void)
{
    btracker_info_base.last_sync_beacon_time = btracker_beacon_meta_info.time;

    btracker_info_base.expected_beacon_seq_id = gl_synced_bcn.beacon_seq_id;
    btracker_info_base.expected_class_seq_id = gl_synced_bcn.beacon_class_seq_id;
    btracker_info_base.expected_beacon_classes_num = gl_synced_bcn.beacon_classes_num;
}

/**
 * Start periodical track timer @btracker_track_periodically_timer.
 * (delay @delay_ms to perform beacon tracking)
 */
static void btracker_start_periodical_track_timer(os_uint16 delay_ms)
{
    haddock_assert(timer_not_started(btracker_track_periodically_timer));

    os_timer_reconfig(btracker_track_periodically_timer, this->_pid,
                      SIGNAL_BTRACKER_TRACK_BEACON,
                      delay_ms);
    os_timer_start(btracker_track_periodically_timer);
}

static void btracker_stop_periodical_track_timer(void)
{
    os_timer_stop(btracker_track_periodically_timer);
}

/**
 * Update @btracker_info_base for each scheduled beacon period.
 */
static void btracker_info_base_ticktock(void)
{
    btracker_info_base.expected_beacon_seq_id += 1;
    if (btracker_info_base.expected_beacon_seq_id == BEACON_OVERFLOW_SEQ_NUM) {
        btracker_info_base.expected_beacon_seq_id = 0;
    }

    btracker_info_base.expected_class_seq_id += 1;
    if (btracker_info_base.expected_class_seq_id >
        btracker_info_base.expected_beacon_classes_num) {
        btracker_info_base.expected_class_seq_id = 1;
    }
}

/**
 * \note Make sure to run btracker_info_base_ticktock() first.
 * \note will change: btracker_info_base::is_check_packed_ack
 * \note will change: btracker_info_base::is_check_join_ack
 */
static os_boolean btracker_is_virtual_track_beacon(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_BCN_TRACKING);

    btracker_info_base.is_check_packed_ack = OS_FALSE;
    btracker_info_base.is_check_join_ack = OS_FALSE;

#ifdef LPWAN_DEBUG_ONLY_TRACK_BEACON
    return OS_FALSE;
#else
    os_boolean is_virtual_tracking = OS_TRUE;

    if (mac_tx_frames_has_pending_tx()
        && mac_engine_is_allow_tx(btracker_info_base.expected_class_seq_id)) {
        // has pending frames to be sent
        is_virtual_tracking = OS_FALSE;
    }


    if (mac_tx_frames_has_waiting_ack(btracker_info_base.expected_beacon_seq_id)) {
        // has frames awaiting ACK
        btracker_info_base.is_check_packed_ack = OS_TRUE;
        if (mac_tx_frames_has_waiting_join_confirm(btracker_info_base.expected_beacon_seq_id)) {
            btracker_info_base.is_check_join_ack = OS_TRUE;
        }
        is_virtual_tracking = OS_FALSE;
    }

    return is_virtual_tracking;
#endif
}

#define MAX_BCN_SPAN_MS                                                                 \
    (btracker_beacon_info.ratio.slots_beacon /* how many slots for beacon */            \
    * btracker_beacon_info.beacon_section_length_us /* each slot length (unit: us) */   \
    / 1000)

#define MAX_BCN_PROCESSING_DELAY    50

/**
 * Calculate the beacon's on-the-air time.
 * \note A approximate calculation due to routinely radio rx behavior.
 * \note The deviation is about Tsym*LORA_SETTINGS_RX_SYMBOL_TIMEOUT, for example
 *       as to <SF8, 125khz>, Tsym is 2ms, and LORA_SETTINGS_RX_SYMBOL_TIMEOUT is 7,
 *       then the deviation is around 14ms.
 *
 * \sa LORA_SETTINGS_RX_SYMBOL_TIMEOUT
 */
static os_uint16 btracker_calc_beacon_span(void)
{
    struct time bcn_span;   // beacon span = beacon rx time - beacon tx time
    os_uint32 bcn_span_ms;

    haddock_time_calc_delta(& btracker_info_base.last_sync_beacon_time.tx,
                            & btracker_info_base.last_sync_beacon_time.rx,
                            & bcn_span);
    bcn_span_ms = bcn_span.s * 1000 + bcn_span.ms;

    haddock_assert(bcn_span_ms <= MAX_BCN_SPAN_MS);
    return (os_uint16) bcn_span_ms;
}

/**
 * Calculate the beacon processing delay:
 *  time delta between beacon's rx time and current time.
 */
static os_uint16 btracker_calc_beacon_rx_till_now_delta(void)
{
    struct time delta;
    os_uint32 delta_ms;

    haddock_time_calc_delta_till_now(& btracker_info_base.last_sync_beacon_time.rx,
                                     & delta);
    delta_ms = delta.s * 1000 + delta.ms;

    haddock_assert(delta_ms <= MAX_BCN_PROCESSING_DELAY);
    return (os_uint16) delta_ms;
}

/**
 * Calculate the time delta between beacon's tx time and current time.
 */
static os_uint16 btracker_calc_beacon_tx_till_now_delta(void)
{
    struct time delta;
    os_uint32 delta_ms;

    haddock_time_calc_delta_till_now(& btracker_info_base.last_sync_beacon_time.tx,
                                     & delta);
    delta_ms = delta.s * 1000 + delta.ms;

    haddock_assert(delta_ms <= (MAX_BCN_SPAN_MS + MAX_BCN_PROCESSING_DELAY));
    return (os_uint16) delta_ms;
}

/**
 * A valid beacon has been found, MAC engine request to beacon beacon tracking.
 * \sa SIGNAL_BTRACKER_BEGIN_BEACON_TRACKING
 */
static void btracker_do_begin_bcn_tracking(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_BEACON_FOUND);

    btracker_sync_beacon_frame_info();
    btracker_sync_info_base();

    /** first delta time used to track beacon
     * \sa btracker_calc_beacon_tx_till_now_delta() */
    os_uint16 delta = 1000 * btracker_beacon_info.beacon_period_length_s -
                      btracker_calc_beacon_tx_till_now_delta() -
                      DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS;
    btracker_start_periodical_track_timer(delta);

    gl_btracker_states = BCN_TRACKER_BCN_TRACKING;
}

/**
 * A real beacon tracking is needed. Perform the beacon tracking.
 * \sa btracker_handle_track_bcn_ok() - callback function when a valid beacon is tracked
 * \sa btracker_handle_track_bcn_timeout() - callback function when no valid beacon be tracked
 */
static void btracker_do_bcn_tracking(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_BCN_TRACKING);

    // setup the tracker's timeout timer
    btracker_track_timeout_timer = btracker_timeout_timer;
    os_timer_reconfig(btracker_track_timeout_timer, this->_pid,
                      SIGNAL_BTRACKER_TRACK_TIMEOUT,
                      DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS);
    os_timer_start(btracker_track_timeout_timer);

    switch_rlc_caller();
    radio_controller_rx_stop(); // explicitly put radio to IDLE mode.
    /** A little longer to avoid race condition. */
    radio_controller_rx_continuously(DEVICE_MAC_TRACK_BEACON_TIMEOUT_MS + 10);
}

/**
 * If a valid beacon from the expected gateway has been received, handle the
 * beacon frame, and restart the btracker_track_periodically_timer (synchronize
 * again).
 *
 * @param is_match_expected_beacon \sa btracker_is_match_expected_beacon()
 *
 * \sa btracker_do_bcn_tracking()
 * \sa btracker_do_begin_bcn_tracking()
 */
static void btracker_handle_track_bcn_ok(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_BCN_TRACKING
                   || gl_btracker_states == BCN_TRACKER_BCN_TRACKING_SEARCH);

    btracker_beacon_meta_info.signal_strength = lpwan_radio_get_last_rx_rssi_snr();
    lpwan_radio_get_last_rx_frame_time(& btracker_beacon_meta_info.time);

    btracker_sync_beacon_frame_info();
    btracker_sync_info_base();

    // restart the periodical beacon tracker timer
    os_uint16 delta = 1000 * btracker_beacon_info.beacon_period_length_s -
                      btracker_calc_beacon_tx_till_now_delta() -
                      DEVICE_MAC_TRACK_BEACON_IN_ADVANCE_MS;
    btracker_start_periodical_track_timer(delta);
}

/**
 * If lost beacon during tracking, we stop the periodical beacon tracker, and perform TRACK_SEARCH.
 * If TRACK_SEARCH also failed, we notify MAC engine to decide whether or not continue search.
 * \sa btracker_do_bcn_tracking()
 */
static void btracker_handle_track_bcn_timeout(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_BCN_TRACKING);
    btracker_stop_periodical_track_timer();

    os_uint32 delay_ms = (gl_synced_bcn.beacon_period_length_s * 1000) / 2;
    os_timer_reconfig(btracker_track_timeout_timer, this->_pid,
                      SIGNAL_BTRACKER_BEGIN_TRACK_SEARCH, delay_ms);
    os_timer_start(btracker_track_timeout_timer);

    gl_btracker_states = BCN_TRACKER_BCN_TRACKING_SEARCH;

    /* Notify MAC engine to handle the lost beacon situation */
    os_ipc_set_signal(gl_registered_mac_engine_pid,
                      SIGNAL_MAC_ENGINE_BEACON_TRACK_LOST_BEACON);
}

/**
 * \note We use @btracker_search_timeout_timer to perform TRACK_SEARCH.
 * \sa btracker_handle_track_bcn_timeout()
 * \sa BCN_TRACKER_BCN_TRACKING_SEARCH
 */
static void btracker_do_track_search(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_BCN_TRACKING_SEARCH);

    btracker_search_timeout_timer = btracker_timeout_timer;

    os_uint32 timeout_ms = gl_synced_bcn.beacon_period_length_s * 1000;
    os_timer_reconfig(btracker_search_timeout_timer, this->_pid,
                      SIGNAL_BTRACKER_TRACK_SEARCH_TIMEOUT, timeout_ms);
    os_timer_start(btracker_search_timeout_timer);

    switch_rlc_caller();
    radio_controller_rx_stop();
    /** A little longer to avoid race condition. */
    radio_controller_rx_continuously(timeout_ms + 10);
}

/**
 * \sa btracker_handle_beacon_not_found()
 */
static void btracker_handle_track_search_timeout(void)
{
    haddock_assert(gl_btracker_states == BCN_TRACKER_BCN_TRACKING_SEARCH);
    btracker_reset();
    os_ipc_set_signal(gl_registered_mac_engine_pid, SIGNAL_MAC_ENGINE_BEACON_TRACK_FAILED);
}

/** Return the synced frame header. */
const struct parsed_frame_hdr_info *btracker_get_parsed_frame_hdr_info(void)
{
    return & gl_synced_fhdr;
}

/** Return the synced beacon body's information. */
const struct parsed_beacon_info *btracker_get_parsed_beacon_info(void)
{
    return & gl_synced_bcn;
}

/** Return the synced packed ACK. */
const struct parsed_beacon_packed_ack_to_me *btracker_get_parsed_packed_ack(void)
{
    return & gl_synced_ack;
}

const struct beacon_tracker_info_base *btracker_get_info_base(void)
{
    return & btracker_info_base;
}

const struct lpwan_last_rx_frame_time *btacker_get_last_bcn_time(void)
{
    return & btracker_beacon_meta_info.time;
}

const struct lpwan_last_rx_frame_rssi_snr *btracker_get_last_bcn_signal_strength(void)
{
    return & btracker_beacon_meta_info.signal_strength;
}
