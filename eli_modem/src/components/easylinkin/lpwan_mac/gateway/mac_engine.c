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
#include "protocol_utils/parse_frame_hdr.h"
#include "protocol_utils/parse_device_uplink_msg.h"

#include "radio/lpwan_radio.h"

#include "lib/assert.h"
#include "lib/ringbuffer.h"
#include "simple_log.h"

#include "crc/crc.h"

#include "lpwan_config.h"
#include "config_gateway.h"
#include "mac_engine.h"

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

static os_uint32 _stat_total_try_to_tx_frame_num = 0;
static os_uint32 _stat_total_tx_ok_frame_num = 0;
static os_uint32 _stat_total_tx_timeout_frame_num = 0;

/**< @} */
/*---------------------------------------------------------------------------*/

os_pid_t gl_gateway_mac_engine_pid;
haddock_process("gateway_mac_engine");

static void gateway_mac_get_uuid(gateway_uuid_t *uuid);
static void gateway_mac_srand(void);
static signal_bv_t gateway_mac_engine_entry(os_pid_t pid, signal_bv_t signal);
static void gateway_init_beacon_info(struct lpwan_gateway_mac_info *info);
static os_int8 construct_gateway_beacon_packed_ack(void *buffer,
                                                   os_uint8 buffer_len,
                                                   struct ringbuffer *rbuf);

/**
 * the MAC's rx and tx buffer
 */
static os_uint8 gateway_mac_rx_buffer[1+LPWAN_RADIO_RX_BUFFER_MAX_LEN];
static os_uint8 gateway_mac_tx_buffer[1+LPWAN_RADIO_RX_BUFFER_MAX_LEN];

static struct lpwan_gateway_mac_info mac_info;

void gateway_mac_engine_init(os_uint8 priority)
{
    struct process *proc_mac_engine = process_create(gateway_mac_engine_entry,
                                                     priority);
    haddock_assert(proc_mac_engine);

    gl_gateway_mac_engine_pid = proc_mac_engine->_pid;

    /** initialize the @mac_info @{ */
    haddock_memset(& mac_info, 0, sizeof(mac_info));

    /** @} */

    gateway_mac_get_uuid(& mac_info.gateway_uuid);
    gateway_mac_srand();

    for (os_uint8 i=0; i < GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM; i++) {
        mac_info.packed_ack_delay_list[i] = \
                rbuf_new(GATEWAY_MAX_UPLINK_MSG_PER_BEACON_PERIOD,
                         sizeof(struct beacon_packed_ack));
        haddock_assert(mac_info.packed_ack_delay_list[i] != NULL);
    }

    os_ipc_set_signal(this->_pid, SIGNAL_GW_MAC_ENGINE_INIT_FINISHED);

    lpwan_radio_register_radio_controller_pid(gl_gateway_mac_engine_pid);
    /** initialize the radio interface */
    lpwan_radio_init();
}

static signal_bv_t gateway_mac_engine_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);

    static os_boolean _run_first_time = OS_TRUE;
    static struct timer *beacon_timer = NULL;
    static struct timer *radio_check_timer = NULL;

    static os_uint8 _frame_cnter_per_period;

    if (_run_first_time) {
        beacon_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                       GW_MAC_ENGINE_MAX_TIMER_DELTA_MS);
        haddock_assert(beacon_timer);

        radio_check_timer = os_timer_create(this->_pid, SIGNAL_SYS_MSG,
                                            GW_MAC_ENGINE_MAX_TIMER_DELTA_MS);
        haddock_assert(radio_check_timer);

        _run_first_time = OS_FALSE;
    }

    if (signal & SIGNAL_GW_MAC_ENGINE_INIT_FINISHED) {
        gateway_init_beacon_info(& mac_info);
        mac_info.cur_packed_ack_delay_list_id = 0-GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM;

        os_timer_reconfig(beacon_timer, this->_pid,
                          SIGNAL_GW_MAC_SEND_BEACON,
                          1000 * mac_info.bcn_info.beacon_period_length);

        os_timer_reconfig(radio_check_timer, this->_pid,
                          SIGNAL_GW_MAC_ENGINE_CHECK_RADIO_TIMEOUT,
                          GATEWAY_MAC_RADIO_PERIODICAL_CHECK_INTERVAL);

        os_timer_start(beacon_timer);
        os_timer_start(radio_check_timer);

        return signal ^ SIGNAL_GW_MAC_ENGINE_INIT_FINISHED;
    }

    if (signal & SIGNAL_GW_MAC_ENGINE_CHECK_RADIO_TIMEOUT) {
        lpwan_radio_routine();
        os_timer_reconfig(radio_check_timer, this->_pid,
                          SIGNAL_GW_MAC_ENGINE_CHECK_RADIO_TIMEOUT,
                          GATEWAY_MAC_RADIO_PERIODICAL_CHECK_INTERVAL);
        os_timer_start(radio_check_timer);
        return signal ^ SIGNAL_GW_MAC_ENGINE_CHECK_RADIO_TIMEOUT;
    }

    if (signal & SIGNAL_LPWAN_RADIO_TX_OK) {
        print_log(LOG_INFO, "tx ok");
        _stat_total_tx_ok_frame_num += 1;
        lpwan_radio_start_rx();
        return signal ^ SIGNAL_LPWAN_RADIO_TX_OK;
    }
    
    if (signal & SIGNAL_LPWAN_RADIO_TX_TIMEOUT) {
        print_log(LOG_ERROR, "tx timeout");
        _stat_total_tx_timeout_frame_num += 1;
        lpwan_radio_start_rx();
        return signal ^ SIGNAL_LPWAN_RADIO_TX_TIMEOUT;
    }

    if (signal & SIGNAL_LPWAN_RADIO_RX_TIMEOUT) {
        lpwan_radio_start_rx();
        return signal ^ SIGNAL_LPWAN_RADIO_RX_TIMEOUT;
    }

    if (signal & SIGNAL_LPWAN_RADIO_RX_CRC_ERROR) {
        lpwan_radio_start_rx();
        print_log(LOG_WARNING, "rx crc error");
        return signal ^ SIGNAL_LPWAN_RADIO_RX_CRC_ERROR;
    }

    if (signal & SIGNAL_LPWAN_RADIO_RX_OK) {

        static os_uint8 *_rx_buf = gateway_mac_rx_buffer;
        static struct parsed_frame_hdr_info _f_hdr;
        static struct beacon_packed_ack _ack;
        static struct parsed_device_uplink_msg_info _msg_info;
        static os_int8 expected_packed_ack_list_id;

        lpwan_radio_read(_rx_buf, 1+LPWAN_RADIO_RX_BUFFER_MAX_LEN);
        // we don't wait, just restart the radio's rx process
        lpwan_radio_start_rx();

        if (_rx_buf[0] == 0
            || 0 > lpwan_parse_frame_header((const struct frame_header *)(_rx_buf+1),
                                             _rx_buf[0],
                                             & _f_hdr)
            // we only handle packets from end-device with valid destination address
            || _f_hdr.frame_origin_type != DEVICE_END_DEVICE
            || _f_hdr.dest.type != ADDR_TYPE_SHORT_ADDRESS
            || _f_hdr.dest.addr.short_addr != mac_info.gateway_cluster_addr
            // we only handle valid source address (short_addr_t and modem_uuid_t)
            || _f_hdr.src.type == ADDR_TYPE_MULTICAST_ADDRESS
            || _f_hdr.src.type == ADDR_TYPE_SHORTENED_MODEM_UUID)
        {
            print_log(LOG_WARNING, "gw: invalid frame!");
            goto gateway_mac_label_radio_rx_invalid_frame;
        }

        switch ((int) _f_hdr.info.de.frame_type) {
        case FTYPE_DEVICE_JOIN         :
        case FTYPE_DEVICE_REJOIN       :
            if (_f_hdr.src.type != ADDR_TYPE_MODEM_UUID)
                goto gateway_mac_label_radio_rx_invalid_frame;
            // todo
            break;
        case FTYPE_DEVICE_DATA_REQUEST :
        case FTYPE_DEVICE_ACK          :
        case FTYPE_DEVICE_CMD          :
            if (_f_hdr.src.type != ADDR_TYPE_SHORT_ADDRESS)
                goto gateway_mac_label_radio_rx_invalid_frame;
            // todo
            break;
        case FTYPE_DEVICE_MSG          :
            if (_f_hdr.src.type != ADDR_TYPE_SHORT_ADDRESS)
                goto gateway_mac_label_radio_rx_invalid_frame;

            _ack.hdr = 0x00;
            set_bits(_ack.hdr, 7, 7, OS_FALSE); // @is_join_ack
            set_bits(_ack.hdr, 2, 1, RADIO_TX_POWER_LEVELS_NUM-1); // @preferred_next_tx_power
            _ack.addr.short_addr = _f_hdr.src.addr.short_addr;

            if (0 == lpwan_parse_device_uplink_msg(
                            (struct device_uplink_msg *)(_rx_buf+1+FRAME_HDR_LEN_NORMAL),
                            _rx_buf[0]-FRAME_HDR_LEN_NORMAL,
                            &_msg_info)) // check if is a valid uplink msg
            {
                _frame_cnter_per_period += 1;
                print_log(LOG_INFO, "gw: rx (%04x)", (os_uint16) _f_hdr.src.addr.short_addr);
                _ack.confirmed_seq = _msg_info.seq;
                expected_packed_ack_list_id = (mac_info.cur_packed_ack_delay_list_id +
                                               mac_info.bcn_info.packed_ack_delay_num)
                                               % mac_info.bcn_info.packed_ack_delay_num;
                rbuf_push_back(mac_info.packed_ack_delay_list[expected_packed_ack_list_id],
                               &_ack, sizeof(struct beacon_packed_ack));
            }


            break;
        }

gateway_mac_label_radio_rx_invalid_frame:
        return signal ^ SIGNAL_LPWAN_RADIO_RX_OK;
    }
    
    if (signal & SIGNAL_GW_MAC_SEND_BEACON) {
        lpwan_radio_stop_rx();
        _frame_cnter_per_period = 0;
        
        /** restart the beacon timer @{ */
        os_timer_reconfig(beacon_timer, this->_pid,
                SIGNAL_GW_MAC_SEND_BEACON,
                1000 * mac_info.bcn_info.beacon_period_length);
        os_timer_start(beacon_timer);
        /** @} */
        
        /** update the beacon information. @{ */
        // class seq id
        mac_info.bcn_info.beacon_class_seq_id += 1;
        if (mac_info.bcn_info.beacon_class_seq_id > mac_info.bcn_info.beacon_classes_num) {
            mac_info.bcn_info.beacon_class_seq_id = 1;
        }
        // beacon seq id
        mac_info.bcn_info.beacon_seq_id += 1;
        if (mac_info.bcn_info.beacon_seq_id >= BEACON_MAX_SEQ_NUM) {
            mac_info.bcn_info.beacon_seq_id = 0;
            // beacon group id
            mac_info.bcn_info.beacon_group_seq_id += 1;
            if (mac_info.bcn_info.beacon_group_seq_id > mac_info.bcn_info.beacon_groups_num) {
                mac_info.bcn_info.beacon_group_seq_id = 0;
            }
        }
        /** @} */

        /** update the packed ack information. @{ */
        mac_info.cur_packed_ack_delay_list_id += 1;
        if (mac_info.cur_packed_ack_delay_list_id >
            (os_int8) (mac_info.bcn_info.packed_ack_delay_num - 1)) {
            mac_info.cur_packed_ack_delay_list_id = 0;
        }
        /** @} */

        // construct the frame header
        _frame_src.type = ADDR_TYPE_SHORT_ADDRESS;
        _frame_src.addr.short_addr = mac_info.gateway_cluster_addr;
        _frame_dest.type = ADDR_TYPE_MULTICAST_ADDRESS;
        _frame_dest.addr.multi_addr = LPWAN_SYS_ADDR_MULTICAST_ALL;

        gateway_mac_tx_buffer[0] = 0;

        os_int8 _len = 0;
        _len = construct_gateway_frame_header(
                    gateway_mac_tx_buffer + 1,
                    LPWAN_RADIO_RX_BUFFER_MAX_LEN,
                    FTYPE_GW_BEACON,
                    & _frame_src, & _frame_dest,
                    OS_TRUE,
                    OS_FALSE, _beacon_period_section_none);
        haddock_assert(_len > 0);
        gateway_mac_tx_buffer[0] += _len;

        // construct the beacon header by @mac_info.bcn_info
        mac_info.bcn_info.has_packed_ack = OS_FALSE;
        if (mac_info.cur_packed_ack_delay_list_id >= 0) {
            if (mac_info.packed_ack_delay_list[mac_info.cur_packed_ack_delay_list_id]->hdr.len > 0) {
                mac_info.bcn_info.has_packed_ack = OS_TRUE;
            }
        }
        _len = construct_gateway_beacon_header(
                    gateway_mac_tx_buffer + 1 + gateway_mac_tx_buffer[0],
                    LPWAN_RADIO_RX_BUFFER_MAX_LEN - 1 - gateway_mac_tx_buffer[0],
                    & mac_info.bcn_info);
        haddock_assert(_len > 0);
        gateway_mac_tx_buffer[0] += _len;

        // construct the beacon's packed ack part by @mac_info.packed_ack_delay_list
        os_uint8 _packed_ack_num = 0;
        if (mac_info.bcn_info.has_packed_ack == OS_TRUE) {
            _packed_ack_num = mac_info.packed_ack_delay_list[mac_info.cur_packed_ack_delay_list_id]->hdr.len;
            _len = construct_gateway_beacon_packed_ack(
                        gateway_mac_tx_buffer + 1 + gateway_mac_tx_buffer[0],
                        LPWAN_RADIO_RX_BUFFER_MAX_LEN - 1 - gateway_mac_tx_buffer[0],
                        mac_info.packed_ack_delay_list[mac_info.cur_packed_ack_delay_list_id]);
            haddock_assert(_len >= 0);
            gateway_mac_tx_buffer[0] += _len;
        }

        /** Transmit the beacon, and wait SIGNAL_LPWAN_RADIO_TX_OK to restart the radio
         *  rx again to receive frames. @{ */
        // haddock_assert(gateway_mac_tx_buffer[0] <= LPWAN_RADIO_TX_MAX_LEN);
        haddock_assert(0 == lpwan_radio_tx(gateway_mac_tx_buffer+1, (os_uint16) gateway_mac_tx_buffer[0]));
        if (mac_info.bcn_info.has_packed_ack == OS_TRUE) {
            print_log(LOG_INFO, "gw: beacon(%d) %s %d",
                      mac_info.bcn_info.beacon_seq_id, "+ACK", _packed_ack_num);
        } else {
            print_log(LOG_INFO, "gw: beacon(%d)", mac_info.bcn_info.beacon_seq_id);
        }
        _stat_total_try_to_tx_frame_num += 1;
        /** @} */

        return signal ^ SIGNAL_GW_MAC_SEND_BEACON;
    }

    print_log(LOG_ERROR, "unknown signal");
    // discard unkown signals.
    return 0;
}

static void gateway_init_beacon_info(struct lpwan_gateway_mac_info *info)
{
    /**< todo @{ */
    info->bcn_info.gateway_cluster_addr = 0x0000;

    info->bcn_info.required_min_version = 0x0;
    info->bcn_info.lpwan_protocol_version = 0x0;

    info->bcn_info.allowable_max_tx_power = RADIO_TX_POWER_LEVELS_NUM-1;

    info->bcn_info.is_join_allowed = OS_TRUE;
    info->bcn_info.is_server_connected = OS_TRUE;
    info->bcn_info.occupied_capacity = GW_OCCUPIED_CAPACITY_BELOW_25;

    info->bcn_info.nearby_channels = 0x00;
    /**< @} */

    info->bcn_info.has_packed_ack = OS_FALSE;

    info->bcn_info.packed_ack_delay_num = GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM;

    info->bcn_info._beacon_period_length = BEACON_PERIOD_2S;
    info->bcn_info._beacon_max_groups_num = BEACON_MAX_GROUP_15;
    info->bcn_info.beacon_period_length = _beacon_period_length_list[BEACON_PERIOD_2S];
    info->bcn_info.beacon_groups_num = _beacon_groups_num_list[BEACON_MAX_GROUP_15];

    info->bcn_info.beacon_classes_num = 1; // use the smallest classes num by default

    info->bcn_info.beacon_seq_id = (os_int8) 0x80;
    info->bcn_info.beacon_group_seq_id = 0;
    info->bcn_info.beacon_class_seq_id = 1; // range [1, @beacon_classes_num]

    /*
     * sum(ratio_xxx) == 128
     * As to the default BEACON_PERIOD_2S, each section is 2000/128 (i.e. 15.625ms).
     *
     * todo
     */
    info->bcn_info.ratio.ratio_beacon       = 6;
    info->bcn_info.ratio.ratio_downlink_msg = 0;
    info->bcn_info.ratio.ratio_uplink_msg   = 122;
}

/**
 * \note this function will change the @rbuf.
 */
static os_int8 construct_gateway_beacon_packed_ack(void *buffer,
                                                   os_uint8 buffer_len,
                                                   struct ringbuffer *rbuf)
{
    if (buffer_len < sizeof(os_uint8) + (rbuf->hdr.blk_size * rbuf->hdr.len))
        return -1;

    os_uint8 *_buf = buffer;
    _buf[0] = rbuf->hdr.len;

    struct beacon_packed_ack _ack;
    for (os_size_t i=0; i < _buf[0]; ++i) {
        rbuf_pop_front(rbuf, &_ack, sizeof(struct beacon_packed_ack));
        haddock_memcpy(_buf + 1 + i*sizeof(struct beacon_packed_ack),
                       &_ack, sizeof(struct beacon_packed_ack));
    }
    haddock_assert(rbuf->hdr.len == 0);

    return sizeof(os_uint8) + (rbuf->hdr.blk_size * _buf[0]);
}

/**
 * Get the UUID of gateway (may generate from the unique ID of gateway's CPU).
 */
static void gateway_mac_get_uuid(gateway_uuid_t *uuid)
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
}

/**
 * Call gateway_mac_get_uuid() first.
 */
static void gateway_mac_srand(void)
{
    os_int32 seed = construct_u32_4(mac_info.gateway_uuid.addr[7],
                                    mac_info.gateway_uuid.addr[6],
                                    mac_info.gateway_uuid.addr[5],
                                    mac_info.gateway_uuid.addr[4]);
    hdk_srand(seed);
}
