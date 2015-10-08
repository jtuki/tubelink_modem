/**
 * mac_engine.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "kernel/process.h"
#include "kernel/ipc.h"
#include "kernel/hdk_memory.h"
#include "kernel/timer.h"
#include "kernel/sys_signal_defs.h"

#include "frame_defs/frame_defs.h"
#include "protocol_utils/construct_frame_hdr.h"
#include "protocol_utils/construct_gw_beacon.h"
#include "protocol_utils/construct_gw_join_response.h"
#include "protocol_utils/parse_frame_hdr.h"
#include "protocol_utils/parse_device_uplink_msg.h"


#include "lib/assert.h"
#include "lib/ringbuffer.h"
#include "simple_log.h"

#include "radio/lpwan_radio.h"
#include "radio_controller/radio_controller.h"

#include "packed_ack_frames_mgmt/packed_ack_frames_mgmt.h"

#include "lpwan_config.h"
#include "config_gateway.h"
#include "mac_engine.h"
#include "mac_engine_driver.h"

os_pid_t gl_gateway_mac_engine_pid;
haddock_process("gateway_mac_engine");

static enum gw_mac_states gl_gw_mac_engine_state;

static struct timer *gl_beacon_timer = NULL;
static struct timer *gl_timeout_timer = NULL;   /**< tx downlink frames' timer etc. */

static signal_bv_t gateway_mac_engine_entry(os_pid_t pid, signal_bv_t signal);

static void gateway_mac_engine_start(void);
static void gateway_mac_engine_stop(void);
static void gw_mac_restart_periodical_bcn_timer(os_uint16 delay_ms);

static signal_t gw_mac_handle_driver_related_signals(signal_bv_t signals);

static void gw_mac_engine_update_beacon_info(void);
static void gateway_init_beacon_info(struct lpwan_gateway_mac_info *info);
static void gw_mac_engine_bcn_info_ticktock(void);

static os_uint8 gw_mac_engine_construct_beacon_frame(void);
static os_int8 construct_gateway_beacon_packed_ack(os_uint8 *buffer,
                                                   os_uint8 buffer_len,
                                                   os_uint8 packed_ack_num);

static signal_t gw_mac_handle_tx_bcn_failed(os_int8 bcn_seq_id, signal_bv_t signals);
static void gw_mac_handle_tx_bcn_ok(os_int8 bcn_seq_id, os_uint8 try_tx_packed_ack_num);

static void gw_mac_do_tx_downlink_frame(const struct gw_downlink_fbuf *fbuf);
static signal_t gw_mac_handle_tx_downlink_frame_failed(const struct gw_downlink_fbuf *fbuf,
                                                       signal_bv_t signals);
static void gw_mac_handle_tx_downlink_frame_ok(const struct gw_downlink_fbuf *fbuf);

static void gw_mac_do_continue_tx_or_begin_rx(void);

static void gw_mac_do_begin_rx_uplink(void);

static void gw_mac_handle_uplink_frames(os_uint8 *rx_buf, os_uint8 buf_len);
static os_int8 gw_mac_handle_uplink_join_req(struct device_join_request *join_req,
                                             os_uint8 len, const modem_uuid_t *uuid);
static os_int8 gw_mac_handle_uplink_msg(struct device_uplink_msg *msg, os_uint8 len,
                                        short_addr_t src_addr);

static os_uint16 calc_span_remains(struct time *t, os_uint16 span_ms);
static os_uint16 gw_mac_calc_beacon_span_remains(void);
static os_uint16 gw_mac_calc_tx_downlink_span_remains(void);

static void gateway_mac_get_uuid(modem_uuid_t *uuid);
static void gateway_mac_srand(void);

static os_boolean gw_mac_is_valid_tx_bcn_seq_id(os_int8 tx_bcn_seq_id);
static os_int8 calc_expected_beacon_seq_id(os_int8 cur_seq_id,
                                           os_int8 packed_ack_delay_num);

/**
 * Gateway MAC engine's tx buffer.
 *
 * \sa gl_radio_rx_buffer
 * \sa ecp_gw_m2c_tx_buf
 */
static os_uint8 gl_gw_mac_tx_beacon_buf[1+LPWAN_RADIO_TX_BUFFER_MAX_LEN];
static const struct gw_downlink_fbuf *gl_cur_tx_downlink_fbuf = NULL;

static struct lpwan_last_rx_frame_rssi_snr gl_last_rx_signal_strength;

static struct lpwan_gateway_mac_info mac_info;

void gateway_mac_engine_init(os_uint8 priority)
{
    struct process *proc_mac_engine = process_create(gateway_mac_engine_entry,
                                                     priority);
    haddock_assert(proc_mac_engine);

    gl_gateway_mac_engine_pid = proc_mac_engine->_pid;


    gl_beacon_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                           GW_MAC_ENGINE_MAX_TIMER_DELTA_MS);
    haddock_assert(gl_beacon_timer);

    gl_timeout_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                               GW_MAC_ENGINE_MAX_TIMER_DELTA_MS);
    haddock_assert(gl_timeout_timer);

    // init gateway modem's mac_info
    haddock_memset(& mac_info, 0, sizeof(mac_info));

    // obtain the Gateway's modem UUID.
    gateway_mac_get_uuid(& mac_info.gateway_modem_uuid);
    gateway_mac_srand();

    packed_ack_mgmt_alloc_buffer();

    gl_gw_mac_engine_state = GW_MAC_STATE_INITED;

#if defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_CPU_CONNECTED
    // wait for the connected CPU to activate gateway modem
#elif defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_AUTO_TEST
    // automatically initialize MAC engine.
    os_ipc_set_signal(this->_pid, SIGNAL_GW_MAC_ENGINE_START);
#endif
}

static signal_bv_t gateway_mac_engine_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);

    static os_uint8 try_tx_packed_ack_num = 0;

    if (signal & SIGNAL_GW_MAC_DRIVER_SIGNALS) {
        signal_t processed_signal = gw_mac_handle_driver_related_signals(signal);
        return signal ^ processed_signal;
    }

    switch (gl_gw_mac_engine_state) {
    case GW_MAC_STATE_INITED:
        if (signal & SIGNAL_GW_MAC_SEND_BEACON) {
            // change state to GW_MAC_STATE_TX_BEACON and re-raise the signal.
            gl_gw_mac_engine_state = GW_MAC_STATE_TX_BEACON;
            os_ipc_set_signal(this->_pid, SIGNAL_GW_MAC_SEND_BEACON);
            return signal ^ SIGNAL_GW_MAC_SEND_BEACON;
        }
        __should_never_fall_here();
        break;
    case GW_MAC_STATE_TX_BEACON:
        if (signal & SIGNAL_GW_MAC_SEND_BEACON) {
            radio_controller_rx_stop();

            /** Restart the periodical beacon timer immediately. */
            gw_mac_restart_periodical_bcn_timer(1000 * mac_info.bcn_info.beacon_period_length_s);

            gw_mac_engine_bcn_info_ticktock();
            packed_ack_ticktock();

            try_tx_packed_ack_num = gw_mac_engine_construct_beacon_frame();

            switch_rlc_caller();
            radio_controller_tx_immediately(gl_gw_mac_tx_beacon_buf+1,
                                            (os_uint8) gl_gw_mac_tx_beacon_buf[0]);

            return signal ^ SIGNAL_GW_MAC_SEND_BEACON;
        }

        if (signal & SIGNALS_RLC_TX_FAILED) {
            signal_t processed_signal = \
                    gw_mac_handle_tx_bcn_failed(mac_info.bcn_info.beacon_seq_id, signal);
            return signal ^ processed_signal;
        }

        if (signal & SIGNAL_RLC_TX_OK) {
            gw_mac_handle_tx_bcn_ok(mac_info.bcn_info.beacon_seq_id, try_tx_packed_ack_num);
            return signal ^ SIGNAL_RLC_TX_OK;
        }

        __should_never_fall_here();
        break;
    case GW_MAC_STATE_TX_DOWNLINK:
        if (signal & SIGNAL_GW_MAC_TX_DOWNLINK_FRAME) {
            gl_cur_tx_downlink_fbuf = downlink_frames_tx_list_get_next();
            haddock_assert(gl_cur_tx_downlink_fbuf);

            gw_mac_do_tx_downlink_frame(gl_cur_tx_downlink_fbuf);

            return signal ^ SIGNAL_GW_MAC_TX_DOWNLINK_FRAME;
        }

        if (signal & SIGNALS_RLC_TX_FAILED) {
            signal_t processed_signal = \
                    gw_mac_handle_tx_downlink_frame_failed(gl_cur_tx_downlink_fbuf, signal);

            gw_mac_do_continue_tx_or_begin_rx();
            return signal ^ processed_signal;
        }

        if (signal & SIGNAL_RLC_TX_OK) {
            gw_mac_handle_tx_downlink_frame_ok(gl_cur_tx_downlink_fbuf);

            gw_mac_do_continue_tx_or_begin_rx();
            return signal ^ SIGNAL_RLC_TX_OK;
        }

        __should_never_fall_here();
        break;
    case GW_MAC_STATE_RX_UPLINK:
        if (signal & SIGNAL_GW_MAC_BEGIN_RX_UPLINK) {
            /** \note
             * When the @gl_beacon_timer timeout, MAC engine will also stop rx. */
            switch_rlc_caller();
            radio_controller_rx_continuously(MAX_OS_UINT16);

            return signal ^ SIGNAL_GW_MAC_BEGIN_RX_UPLINK;
        }

        if (signal & SIGNAL_RLC_RX_OK) {
            gl_last_rx_signal_strength = lpwan_radio_get_last_rx_rssi_snr();
            gw_mac_handle_uplink_frames(gl_radio_rx_buffer, 1+LPWAN_RADIO_RX_BUFFER_MAX_LEN);

            return signal ^ SIGNAL_LPWAN_RADIO_RX_OK;
        }

        if (signal & SIGNAL_RLC_RX_DURATION_TIMEOUT) {
            // beacon period cannot exceed MAX_OS_UINT16 ms.
            __should_never_fall_here();

            return signal ^ SIGNAL_RLC_RX_DURATION_TIMEOUT;
        }

        if (signal & SIGNAL_GW_MAC_SEND_BEACON) {
            radio_controller_rx_stop();

            // change state to TX_BEACON and re-raise the signal.
            gl_gw_mac_engine_state = GW_MAC_STATE_TX_BEACON;
            os_ipc_set_signal(this->_pid, SIGNAL_GW_MAC_SEND_BEACON);

            return signal ^ SIGNAL_GW_MAC_SEND_BEACON;
        }

        __should_never_fall_here();
        break;
    }

    // discard unkown signals.
    return 0;
}

/**
 * Start the gateway's MAC engine.
 */
static void gateway_mac_engine_start(void)
{
    /** initialize the radio interface */
    lpwan_radio_init();

    gateway_init_beacon_info(& mac_info);

    packed_ack_mgmt_reset_buffer();
    packed_ack_mgmt_init_list_id(0 - ((os_int8) mac_info.bcn_info.packed_ack_delay_num));

    downlink_frames_mgmt_reset();

    gl_gw_mac_engine_state = GW_MAC_STATE_TX_BEACON;
    gw_mac_restart_periodical_bcn_timer(1000 * mac_info.bcn_info.beacon_period_length_s);
}

/**
 * Stop the gateway's MAC engine.
 *
 * \note We don't need to reset the packed ACK and downlink frames buffer.
 *       Because gateway_mac_engine_start() will reset these buffer.
 */
static void gateway_mac_engine_stop(void)
{
    rlc_reset();

    os_timer_stop(gl_beacon_timer);
    os_timer_stop(gl_timeout_timer);

    // packed_ack_mgmt_reset_buffer();
    // downlink_frames_mgmt_reset();

    gl_gw_mac_engine_state = GW_MAC_STATE_INITED;
}

static signal_t gw_mac_handle_driver_related_signals(signal_bv_t signals)
{
    signal_t processed_signal;
    if (signals & SIGNAL_GW_MAC_ENGINE_START) {
        if (! is_gw_mac_engine_started()) {
            gateway_mac_engine_start();
        }

        processed_signal = (signal_t) SIGNAL_GW_MAC_ENGINE_START;
    }
    else if (signals & SIGNAL_GW_MAC_ENGINE_STOP) {
        if (is_gw_mac_engine_started()) {
            gateway_mac_engine_stop();
        }

        processed_signal = (signal_t) SIGNAL_GW_MAC_ENGINE_STOP;
    }
    else if (signals & SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_INFO) {
        /* update information from MAC engine driver's configuration. */
        gw_mac_engine_update_beacon_info();

        processed_signal = (signal_t) SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_INFO;
    }
    else if (signals & SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_CLASSES_NUM) {
        mac_driver_config_get(ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM,
                              & mac_info.bcn_info.beacon_classes_num);

        processed_signal = (signal_t) SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_CLASSES_NUM;
    }
    else if (signals & SIGNAL_GW_MAC_ENGINE_UPDATE_MODEM_CHANNEL) {
        os_uint8 prev_channel = mac_info.modem_channel;
        mac_driver_config_get(ECP_GW_C2M_UPDATE_MODEM_CHANNEL,
                              & mac_info.modem_channel);

        if (prev_channel != mac_info.modem_channel) {
            lpwan_radio_change_base_frequency(
                        lpwan_radio_channels_list[mac_info.modem_channel]);

            if (is_gw_mac_engine_started()) {
                gateway_mac_engine_stop();
                gateway_mac_engine_start();
            }
        }

        processed_signal = (signal_t) SIGNAL_GW_MAC_ENGINE_UPDATE_MODEM_CHANNEL;
    }
    else {
        __should_never_fall_here();
    }

    return processed_signal;
}

static void gw_mac_handle_uplink_frames(os_uint8 *rx_buf, os_uint8 buf_len)
{
    static struct parsed_frame_hdr_info f_hdr;

    if (rx_buf[0] == 0
        || 0 > lpwan_parse_frame_header((const struct frame_header *)(rx_buf+1),
                                         rx_buf[0],
                                         & f_hdr)
        // we only handle packets from end-device with valid destination address
        || f_hdr.frame_origin_type != DEVICE_END_DEVICE
        || f_hdr.dest.type != ADDR_TYPE_SHORT_ADDRESS
        || f_hdr.dest.addr.short_addr != mac_info.gateway_cluster_addr
        // we only handle valid source address (short_addr_t and modem_uuid_t)
        || f_hdr.src.type == ADDR_TYPE_MULTICAST_ADDRESS
        || f_hdr.src.type == ADDR_TYPE_SHORTENED_MODEM_UUID)
    {
        print_log(LOG_WARNING, "GW: invalid frame");
        goto gw_mac_handle_uplink_frame_return;
    }

    switch ((int) f_hdr.info.de.frame_type) {
    case FTYPE_DEVICE_JOIN         :
        if (f_hdr.src.type != ADDR_TYPE_MODEM_UUID)
            goto gw_mac_handle_uplink_frame_return;

        if (0 == gw_mac_handle_uplink_join_req((struct device_join_request *)(rx_buf+1+FRAME_HDR_LEN_JOIN),
                                         rx_buf[0] - FRAME_HDR_LEN_JOIN,
                                         & f_hdr.src.addr.uuid))
        {
#if defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_CPU_CONNECTED
            /** relay the currently received uplink message to gateway CPU */
            ecp_gw_modem_m2c_send_data(rx_buf + 1, rx_buf[0],
                                       gl_last_rx_signal_strength.snr, gl_last_rx_signal_strength.rssi);
#elif defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_AUTO_TEST
            const modem_uuid_t *uuid = & f_hdr.src.addr.uuid;
            print_log(LOG_INFO,
                      "GW: rx JOIN_REQ (%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X)",
                      uuid->addr[0], uuid->addr[1], uuid->addr[2], uuid->addr[3],
                      uuid->addr[4], uuid->addr[5], uuid->addr[6], uuid->addr[7],
                      uuid->addr[8], uuid->addr[9], uuid->addr[10], uuid->addr[11]);
#endif
        }
        break;
    case FTYPE_DEVICE_DATA_REQUEST :
    case FTYPE_DEVICE_ACK          :
    case FTYPE_DEVICE_CMD          :
        if (f_hdr.src.type != ADDR_TYPE_SHORT_ADDRESS)
            goto gw_mac_handle_uplink_frame_return;
        // #mark# - don't handle these frame types from end-device
        break;
    case FTYPE_DEVICE_MSG          :
        if (f_hdr.src.type != ADDR_TYPE_SHORT_ADDRESS)
            goto gw_mac_handle_uplink_frame_return;

        if (0 == gw_mac_handle_uplink_msg((struct device_uplink_msg *)(rx_buf+1+FRAME_HDR_LEN_NORMAL),
                                 rx_buf[0] - FRAME_HDR_LEN_NORMAL,
                                 f_hdr.src.addr.short_addr))
        {
#if defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_CPU_CONNECTED
            /** relay the currently received uplink message to gateway CPU */
            ecp_gw_modem_m2c_send_data(rx_buf + 1, rx_buf[0],
                                       gl_last_rx_signal_strength.snr, gl_last_rx_signal_strength.rssi);
#elif defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_AUTO_TEST
            print_log(LOG_INFO, "GW: rx (%04X)", (os_uint16) f_hdr.src.addr.short_addr);
#endif
        }
        break;
    }

gw_mac_handle_uplink_frame_return:
    return;
}

/**
 * If a downlink frame is required to tx at @tx_bcn_seq_id, is the seq id valid?
 */
static os_boolean gw_mac_is_valid_tx_bcn_seq_id(os_int8 tx_bcn_seq_id)
{
    os_int16 bcn_seq_delta = calc_bcn_seq_delta(tx_bcn_seq_id, mac_info.bcn_info.beacon_seq_id);
    if (0 < bcn_seq_delta && bcn_seq_delta <= mac_info.bcn_info.packed_ack_delay_num) {
        return OS_TRUE;
    } else {
        return OS_FALSE;
    }
}

/**
 * Prepare to tx JOIN_CONFIRM frame.
 * \return -1 if @tx_bcn_seq_id is invalid, -2 if downlink frame buffer full. 0 if ok.
 */
os_int8 gw_mac_prepare_tx_join_confirm(const modem_uuid_t *dest_uuid,
                               const struct gw_join_confirmed *join_response,
                               os_int8 tx_bcn_seq_id)
{
    if (! gw_mac_is_valid_tx_bcn_seq_id(tx_bcn_seq_id))
        return -1;

    const struct gw_downlink_fbuf *c_fbuf = downlink_frame_alloc();
    if (! c_fbuf)
        return -2;

    struct lpwan_addr dest;

    dest.type = ADDR_TYPE_MODEM_UUID;
    dest.addr.uuid = *dest_uuid;

    os_int8 frame_len = construct_gateway_join_response((void *) c_fbuf->frame,
                                        LPWAN_RADIO_TX_BUFFER_MAX_LEN,
                                        dest_uuid, mac_info.gateway_cluster_addr, join_response);
    haddock_assert(frame_len > 0);

    if (-1 == downlink_frame_put(c_fbuf, FTYPE_GW_JOIN_CONFIRMED,
                       & dest, c_fbuf->frame, frame_len,
                       mac_info.bcn_info.beacon_seq_id, tx_bcn_seq_id)) {
        // failed - @tx_bcn_seq_id invalid
        downlink_frame_free(c_fbuf);
        return -1;
    } else {
        return 0;
    }
}

static os_int8 gw_mac_handle_uplink_join_req(struct device_join_request *join_req,
                                             os_uint8 len, const modem_uuid_t *uuid)
{
    if (len != sizeof(struct device_join_request))
        return -1;

    struct parsed_device_uplink_common up_common_info;
    lpwan_parse_device_uplink_common(& join_req->hdr, len, & up_common_info);

    if (up_common_info.beacon_seq_id != mac_info.bcn_info.beacon_seq_id
        || up_common_info.beacon_class_seq_id != mac_info.bcn_info.beacon_class_seq_id)
        return -1;

#if defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_CPU_CONNECTED
    (void) uuid;
    return 0;
#elif defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_AUTO_TEST
    /**
     * Virtual MAC server mode.
     * We virtually generate @distributed_short_addr as below.
     */
    struct gw_join_confirmed join_response;

    // construct @join_response
    set_bits(join_response.hdr, 7, 4, JOIN_CONFIRMED_PAID);
    join_response.distributed_short_addr = (short_addr_t) short_modem_uuid(uuid);
    join_response.init_seq_id = 0;

    if (0 == gw_mac_prepare_tx_join_confirm(uuid, & join_response,
                    calc_expected_beacon_seq_id(mac_info.bcn_info.beacon_seq_id,
                                                mac_info.bcn_info.packed_ack_delay_num))) {
        print_log(LOG_INFO, "virtual JOIN_CONFIRM: (%04X)",
                  (os_uint16) join_response.distributed_short_addr);
    } else {
        print_log(LOG_WARNING, "gw_mac_tx_join_confirm() failed.");
    }

    return 0;
#endif
}

/**
 * Check if @msg is a valid uplink message, and push to packed ACK information.
 */
static os_int8 gw_mac_handle_uplink_msg(struct device_uplink_msg *msg, os_uint8 len,
                                        short_addr_t src_addr)
{
    static struct beacon_packed_ack ack;
    static struct parsed_device_uplink_msg_info msg_info;

    if (lpwan_parse_device_uplink_msg(msg, len, & msg_info) == -1)
        return -1;

    /** @beacon_seq_id and @beacon_class_seq_id should be matched! */
    if (msg_info.up_common.beacon_seq_id != mac_info.bcn_info.beacon_seq_id
        || msg_info.up_common.beacon_class_seq_id != mac_info.bcn_info.beacon_class_seq_id)
        return -1;

    // valid uplink msg
    ack.hdr = 0x0;
    set_bits(ack.hdr, 7, 7, OS_FALSE); // @is_join_ack
    set_bits(ack.hdr, 2, 1, RADIO_TX_POWER_LEVELS_NUM-1); // @preferred_next_tx_power
    ack.addr.short_addr = src_addr;
    ack.confirmed_seq = msg_info.seq;

    return packed_ack_push(&ack); // -1 or 0
}

/**
 * Update the beacon information (ticktock per beacon period).
 */
static void gw_mac_engine_bcn_info_ticktock(void)
{
    // class seq id
    mac_info.bcn_info.beacon_class_seq_id += 1;
    if (mac_info.bcn_info.beacon_class_seq_id >= mac_info.bcn_info.beacon_classes_num) {
        mac_info.bcn_info.beacon_class_seq_id = 0;
    }

    // beacon seq id
    mac_info.bcn_info.beacon_seq_id += 1;
    if (mac_info.bcn_info.beacon_seq_id == BEACON_OVERFLOW_SEQ_NUM) {
        mac_info.bcn_info.beacon_seq_id = 0;
    }
}

static signal_t gw_mac_handle_tx_bcn_failed(os_int8 bcn_seq_id, signal_bv_t signals)
{
    signal_t processed_signal;
    if (signals & SIGNAL_RLC_TX_CCA_FAILED) {
        print_log(LOG_WARNING, "rlc_tx beacon (%d) CCA fail", bcn_seq_id);
        processed_signal = SIGNAL_RLC_TX_CCA_FAILED;
    }
    else if (signals & SIGNAL_RLC_TX_CCA_CRC_FAIL) {
        print_log(LOG_WARNING, "rlc_tx beacon (%d) CCA CRC fail", bcn_seq_id);
        processed_signal = SIGNAL_RLC_TX_CCA_CRC_FAIL;
    }
    else if (signals & SIGNAL_RLC_TX_TIMEOUT) {
        print_log(LOG_WARNING, "rlc_tx beacon (%d) timeout", bcn_seq_id);
        processed_signal = SIGNAL_RLC_TX_TIMEOUT;
    } else {
        __should_never_fall_here();
    }

    // clear current possible downlink tx list, and wait for the next beacon's transmission.
    downlink_frame_clear_tx_list();

    return processed_signal;
}

static void gw_mac_handle_tx_bcn_ok(os_int8 bcn_seq_id, os_uint8 try_tx_packed_ack_num)
{
    print_log(LOG_INFO, "GW: beacon(%d) +(%d)ACK", bcn_seq_id, try_tx_packed_ack_num);
    gw_mac_do_continue_tx_or_begin_rx();
}

static void gw_mac_do_tx_downlink_frame(const struct gw_downlink_fbuf *fbuf)
{
    switch_rlc_caller();
    radio_controller_tx_immediately(fbuf->frame, fbuf->len);
}

static signal_t gw_mac_handle_tx_downlink_frame_failed(const struct gw_downlink_fbuf *fbuf,
                                                       signal_bv_t signals)
{
    signal_t processed_signal;

    if (signals & SIGNAL_RLC_TX_CCA_FAILED) {
        processed_signal = SIGNAL_RLC_TX_CCA_FAILED;
    }
    else if (signals & SIGNAL_RLC_TX_CCA_CRC_FAIL) {
        processed_signal = SIGNAL_RLC_TX_CCA_CRC_FAIL;
    }
    else if (signals & SIGNAL_RLC_TX_TIMEOUT) {
        processed_signal = SIGNAL_RLC_TX_TIMEOUT;
    } else {
        __should_never_fall_here();
    }

    if (fbuf->dest.type == ADDR_TYPE_MODEM_UUID) { // join confirmed frame
        const modem_uuid_t *uuid = & fbuf->dest.addr.uuid;
        print_log(LOG_WARNING,
                  "rlc_tx ok - JOIN_CONFIRM (%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X)",
                  uuid->addr[0], uuid->addr[1], uuid->addr[2], uuid->addr[3],
                  uuid->addr[4], uuid->addr[5], uuid->addr[6], uuid->addr[7],
                  uuid->addr[8], uuid->addr[9], uuid->addr[10], uuid->addr[11]);
    } else { // downlink message frame
        print_log(LOG_WARNING, "rlc_tx fail - frame (%04X)", fbuf->dest.addr.short_addr);
    }

    downlink_frames_handle_tx_failed(fbuf);

    return processed_signal;
}

static void gw_mac_handle_tx_downlink_frame_ok(const struct gw_downlink_fbuf *fbuf)
{
    if (fbuf->dest.type == ADDR_TYPE_MODEM_UUID) { // join confirmed frame
        const modem_uuid_t *uuid = & fbuf->dest.addr.uuid;
        print_log(LOG_INFO_COOL,
                  "rlc_tx ok - JOIN_CONFIRM (%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X) ok",
                  uuid->addr[0], uuid->addr[1], uuid->addr[2], uuid->addr[3],
                  uuid->addr[4], uuid->addr[5], uuid->addr[6], uuid->addr[7],
                  uuid->addr[8], uuid->addr[9], uuid->addr[10], uuid->addr[11]);
    } else { // downlink message frame
        print_log(LOG_WARNING, "rlc_tx fail - frame (%04X)", fbuf->dest.addr.short_addr);
    }

    downlink_frames_handle_tx_ok(fbuf);
}

/**
 * Check if there is more pending downlink frames, if yes, continue to tx, otherwise
 * switch to rx uplink frames.
 */
static void gw_mac_do_continue_tx_or_begin_rx(void)
{
    haddock_assert(gl_gw_mac_engine_state == GW_MAC_STATE_TX_BEACON
                   || gl_gw_mac_engine_state == GW_MAC_STATE_TX_DOWNLINK);

    if (downlink_frames_tx_list_len() == 0) {
        // no pending frames to tx, go to rx mode and receive uplink frames.
        gw_mac_do_begin_rx_uplink();
    } else {
        // delay some time until specified time to tx downlink frame
        os_uint16 (*calc_delta)(void);
        calc_delta = (gl_gw_mac_engine_state == GW_MAC_STATE_TX_BEACON) ?
                     gw_mac_calc_beacon_span_remains : gw_mac_calc_tx_downlink_span_remains;

        gl_gw_mac_engine_state = GW_MAC_STATE_TX_DOWNLINK;

        os_uint16 delay_ms = calc_delta();
        if (delay_ms == 0) {
            os_ipc_set_signal(this->_pid, SIGNAL_GW_MAC_TX_DOWNLINK_FRAME);
        } else {
            os_timer_reconfig(gl_timeout_timer, this->_pid, SIGNAL_GW_MAC_TX_DOWNLINK_FRAME,
                              delay_ms);
            os_timer_start(gl_timeout_timer);
        }
    }
}

/**
 * No more pending downlink frames to tx, start to receive the uplink frames.
 */
static void gw_mac_do_begin_rx_uplink(void)
{
    gl_gw_mac_engine_state = GW_MAC_STATE_RX_UPLINK;
    os_ipc_set_signal(this->_pid, SIGNAL_GW_MAC_BEGIN_RX_UPLINK);
}

/**
 * @t is the tx begin time of last frame, @span_ms is specified reserved time span for tx.
 * \return span_ms - (now - tx_begin_time)
 */
static os_uint16 calc_span_remains(struct time *t, os_uint16 span_ms)
{
    struct time till_now;
    os_uint16 till_now_ms;

    haddock_time_calc_delta_till_now(t, & till_now);
    till_now_ms = till_now.s * 1000 + till_now.ms;

    return (till_now_ms >= span_ms) ? 0 : (span_ms - till_now_ms);
}

static os_uint16 gw_mac_calc_beacon_span_remains(void)
{
    struct lpwan_last_tx_frame_time t;
    lpwan_radio_get_last_tx_frame_time(&t);

    return calc_span_remains(& t.tx_begin,
                             mac_info.bcn_info.beacon_section_length_us
                             * mac_info.bcn_info.ratio.slots_beacon
                             / 1000);
}

static os_uint16 gw_mac_calc_tx_downlink_span_remains(void)
{
    struct lpwan_last_tx_frame_time t;
    lpwan_radio_get_last_tx_frame_time(&t);

    return calc_span_remains(& t.tx_begin,
                             mac_info.bcn_info.beacon_section_length_us
                             * LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS
                             / 1000);
}

/**
 * \return packed_ack_num in Beacon frame.
 *
 * \note Make sure to run packed_ack_ticktock() and downlink_frames_check() first.
 */
static os_uint8 gw_mac_engine_construct_beacon_frame(void)
{
    static short_addr_t gw_cluster_addr = 0xFFFF; // init as a invalid address

    os_int8 len;
    gl_gw_mac_tx_beacon_buf[0] = FRAME_HDR_LEN_NORMAL + (os_int8) sizeof(struct beacon_header);

    if (mac_info.gateway_cluster_addr != gw_cluster_addr) {
        struct lpwan_addr gl_frame_dest;
        struct lpwan_addr gl_frame_src;
        /*
         * Re-init the frame header, because frame header has not been initialized,
         * or the mac_info.gateway_cluster_addr has been changed.
         */
        // construct the frame header
        gl_frame_src.type = ADDR_TYPE_SHORT_ADDRESS;
        gl_frame_dest.type = ADDR_TYPE_MULTICAST_ADDRESS;

        gl_frame_src.addr.short_addr = mac_info.gateway_cluster_addr;
        gl_frame_dest.addr.multi_addr = LPWAN_SYS_ADDR_MULTICAST_ALL;

        len = construct_gateway_frame_header(gl_gw_mac_tx_beacon_buf + 1,
                            LPWAN_RADIO_TX_BUFFER_MAX_LEN, FTYPE_GW_BEACON,
                            & gl_frame_src, & gl_frame_dest, OS_TRUE);
        haddock_assert(len > 0);

        gw_cluster_addr = mac_info.gateway_cluster_addr;
    }

    // construct the beacon's packed ack part by @mac_info.packed_ack_delay_list
    os_uint8 packed_ack_num = 0;
    if (mac_info.bcn_info.has_packed_ack == OS_TRUE) {
        packed_ack_num = packed_ack_cur_list_len();
        haddock_assert(packed_ack_num > 0);

        len = construct_gateway_beacon_packed_ack(
                    gl_gw_mac_tx_beacon_buf + 1 + gl_gw_mac_tx_beacon_buf[0],
                    LPWAN_RADIO_TX_BUFFER_MAX_LEN - 1 - gl_gw_mac_tx_beacon_buf[0],
                    packed_ack_num);
        haddock_assert(len >= 0);

        gl_gw_mac_tx_beacon_buf[0] += len;
    }

    // check downlink frames' information and update @mac_info.bcn_info
    os_uint8 tx_len = downlink_frames_tx_list_len();
    if (tx_len > 0) {
        mac_info.bcn_info.ratio.slots_beacon = LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS;
        mac_info.bcn_info.ratio.slots_downlink_msg = tx_len * LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS;
        mac_info.bcn_info.ratio.slots_uplink_msg = \
                LPWAN_BEACON_PERIOD_SLOTS_NUM - (tx_len+1)*LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS;
    }

    // construct the beacon header by @mac_info.bcn_info
    mac_info.bcn_info.has_packed_ack = has_packed_ack();

    len = construct_gateway_beacon_header(
                gl_gw_mac_tx_beacon_buf + 1 + FRAME_HDR_LEN_NORMAL,
                LPWAN_RADIO_TX_BUFFER_MAX_LEN - 1 - FRAME_HDR_LEN_NORMAL,
                & mac_info.bcn_info);
    haddock_assert(len > 0);

    return packed_ack_num;
}

os_boolean is_gw_mac_engine_started(void)
{
    return (gl_gw_mac_engine_state > GW_MAC_STATE_INITED);
}

static void gw_mac_restart_periodical_bcn_timer(os_uint16 delay_ms)
{
    haddock_assert(gl_gw_mac_engine_state == GW_MAC_STATE_TX_BEACON);

    /** Restart the periodical beacon timer immediately. */
    os_timer_reconfig(gl_beacon_timer, this->_pid,
                      SIGNAL_GW_MAC_SEND_BEACON, delay_ms);
    os_timer_start(gl_beacon_timer);
}

const modem_uuid_t *gw_mac_info_get_modem_uuid(void)
{
    return & mac_info.gateway_modem_uuid;
}

/**
 * Get current packed ACK delay number.
 */
os_uint8 gw_mac_info_get_packed_ack_delay_num(void)
{
    return mac_info.bcn_info.packed_ack_delay_num;
}

/**
 * Init the beacon information when mac engine starts.
 * \sa mac_engine_driver_config
 */
static void gateway_init_beacon_info(struct lpwan_gateway_mac_info *info)
{
    info->bcn_info.required_min_version = 0x0;
    info->bcn_info.lpwan_protocol_version = 0x0;

    info->bcn_info.has_packed_ack = OS_FALSE;

    info->bcn_info.packed_ack_delay_num = GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM;

    info->bcn_info.beacon_period_enum = LPWAN_BEACON_PERIOD;
    info->bcn_info.beacon_period_length_s = gl_beacon_period_length_list[LPWAN_BEACON_PERIOD];
    info->bcn_info.beacon_section_length_us = gl_beacon_section_length_us[LPWAN_BEACON_PERIOD];

    info->bcn_info.beacon_seq_id = (os_int8) 0x80;
    info->bcn_info.beacon_class_seq_id = 0; // range [0, @beacon_classes_num)

    /*
     * sum(slots_xyz) == 128
     * \sa gl_beacon_section_length_us
     */
    info->bcn_info.ratio.slots_beacon       = LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS;
    info->bcn_info.ratio.slots_downlink_msg = 0;
    info->bcn_info.ratio.slots_uplink_msg   = \
                LPWAN_BEACON_PERIOD_SLOTS_NUM - LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS;

    /**
     * obtain information from MAC engine driver's configuration.
     * \sa mac_driver_config_set()
     * \sa mac_driver_config_get()
     */
    mac_driver_config_get(ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM,
                          & info->bcn_info.beacon_classes_num);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER,
                          & info->bcn_info.allowable_max_tx_power);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED,
                          & info->bcn_info.is_join_allowed);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED,
                          & info->bcn_info.is_server_connected);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY,
                          & info->bcn_info.occupied_capacity);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR,
                          & info->bcn_info.gateway_cluster_addr);
}

/**
 * Update the beacon info from the gateway modem's MAC engine driver.
 */
static void gw_mac_engine_update_beacon_info(void)
{
    mac_driver_config_get(ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER,
                          & mac_info.bcn_info.allowable_max_tx_power);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED,
                          & mac_info.bcn_info.is_join_allowed);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED,
                          & mac_info.bcn_info.is_server_connected);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY,
                          & mac_info.bcn_info.occupied_capacity);
    mac_driver_config_get(ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR,
                          & mac_info.bcn_info.gateway_cluster_addr);
}

/**
 * \note this function will change the @rbuf.
 */
static os_int8 construct_gateway_beacon_packed_ack(os_uint8 *buffer,
                                                   os_uint8 buffer_len,
                                                   os_uint8 packed_ack_num)
{
    static struct beacon_packed_ack ack;
    if (buffer_len < 1 + ((sizeof(ack) * packed_ack_num)))
        return -1;

    buffer[0] = packed_ack_num;

    os_size_t n = 0;
    os_boolean found;
    for (; packed_ack_pop(& ack) != NULL; ++n) {
        found = downlink_frames_check(bcn_packed_ack_hdr_is_join(ack.hdr), (void *) & ack.addr);
        if (found) {
            /**
             * Reconfig the packed ACK header:
             *  is_msg_pending <= OS_TRUE
             *  estimation_downlink_slots <= LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS
             */
            bcn_packed_ack_hdr_set_pending_msg(& ack.hdr, OS_TRUE,
                                               LPWAN_BEACON_DEFAULT_PER_DOWNLINK_SLOTS);
        }
        // construct the beacon's packed ACK buffer.
        ack.addr.short_addr = os_hton_u16(ack.addr.short_addr);
        haddock_memcpy(buffer + 1 + n*sizeof(ack), & ack, sizeof(ack));
    }
    haddock_assert(n == packed_ack_num
                   && packed_ack_num <= GATEWAY_MAX_UPLINK_MSG_PER_BEACON_PERIOD);

    if (downlink_frames_cur_list_len() > 0) {
        // we expect that the length should be 0
        downlink_frame_clear_cur_list();
    }

    return (os_int8) (1 + ((sizeof(ack) * n)));
}

/**
 * Get the UUID of gateway's modem's UUID.
 */
static void gateway_mac_get_uuid(modem_uuid_t *uuid)
{
    mcu_read_unique_id(uuid);
    print_log(LOG_INFO, "GW: modem %02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X",
                  uuid->addr[0], uuid->addr[1], uuid->addr[2], uuid->addr[3],
                  uuid->addr[4], uuid->addr[5], uuid->addr[6], uuid->addr[7],
                  uuid->addr[8], uuid->addr[9], uuid->addr[10], uuid->addr[11]);
}

/**
 * Call gateway_mac_get_uuid() first.
 */
static void gateway_mac_srand(void)
{
    os_int32 seed = mcu_generate_seed_from_uuid(& mac_info.gateway_modem_uuid);
    hdk_srand(seed);
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
