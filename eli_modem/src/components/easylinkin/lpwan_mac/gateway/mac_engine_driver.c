/**
 * mac_engine_driver.c
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

#include "lib/assert.h"

#include "lpwan_config.h"

#include "mac_engine.h"
#include "mac_engine_driver.h"
#include "external_comm_protocol/ecp_gw_modem.h"

/**
 * The configuration data obtained after interaction with gateway host CPU.
 * \sa "static struct lpwan_gateway_mac_info mac_info" in \file mac_engine.c
 * \sa beacon_info_t
 *
 * \sa mac_driver_config_set()
 * \sa mac_driver_config_get()
 */
static struct {
    enum _gateway_occupied_capacity occupied_capacity;
    os_uint8    modem_channel           ;   /**< \sa LPWAN_MAX_RADIO_CHANNELS_NUM */
    os_uint8    beacon_classes_num      ;   /**< \sa struct beacon_info_t::beacon_classes_num
                                                 range [1, @BEACON_MAX_CLASSES_NUM] */
    os_boolean  is_join_allowed         ;
    os_boolean  is_server_connected     ;
    os_int8     allowable_max_tx_power  ;   /**< \sa range [0, RADIO_TX_POWER_LEVELS_NUM) */
    short_addr_t gateway_cluster_addr   ;   /**< Gateway's short address.
                                                \note Same for all the modems associated with a gateway. */
} mac_engine_driver_config;


/**
 * Callback handlers for external-communication protocol handling.
 */
static os_int8 mac_driver_handle_c2m_control_payload(os_uint8 *control, os_uint16 len);
static os_int8 mac_driver_handle_c2m_data_payload(os_uint8 *data, os_uint16 len);

static signal_bv_t gateway_mac_engine_driver_entry(os_pid_t pid, signal_bv_t signal);

os_pid_t gl_gateway_mac_engine_driver_pid;
haddock_process("gateway_mac_engine_driver");

void gateway_mac_engine_driver_init(os_uint8 priority)
{
    struct process *mac_engine_driver = process_create(gateway_mac_engine_driver_entry,
                                                       priority);
    haddock_assert(mac_engine_driver);
    gl_gateway_mac_engine_driver_pid = mac_engine_driver->_pid;

    // set up the default @mac_engine_driver_config parameters.
    mac_engine_driver_config.beacon_classes_num = 1;
    mac_engine_driver_config.allowable_max_tx_power = RADIO_TX_POWER_LEVELS_NUM-1;
    mac_engine_driver_config.is_join_allowed = OS_TRUE;
    mac_engine_driver_config.is_server_connected = OS_TRUE;
    mac_engine_driver_config.occupied_capacity = GW_OCCUPIED_CAPACITY_BELOW_25;

    mac_engine_driver_config.modem_channel = 0; // default channel

    ecp_gw_modem_init();
    ecp_gw_modem_register_c2m_control_callback(mac_driver_handle_c2m_control_payload);
    ecp_gw_modem_register_c2m_data_callback(mac_driver_handle_c2m_data_payload);
}


static signal_bv_t gateway_mac_engine_driver_entry(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);

#if defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_AUTO_TEST
    return 0; // we don't care about external commands
#endif

    if (signal & SIGNAL_MAC_DRIVER_START_ENGINE) {
        if (! is_gw_mac_engine_started())
            os_ipc_set_signal(gl_gateway_mac_engine_pid, SIGNAL_GW_MAC_ENGINE_START);
        return signal ^ SIGNAL_MAC_DRIVER_START_ENGINE;
    }

    if (signal & SIGNAL_MAC_DRIVER_STOP_ENGINE) {
        if (is_gw_mac_engine_started()) {
            os_ipc_set_signal(gl_gateway_mac_engine_pid, SIGNAL_GW_MAC_ENGINE_STOP);
        }
        return signal ^ SIGNAL_MAC_DRIVER_STOP_ENGINE;
    }

    if (signal & SIGNAL_MAC_DRIVER_BCN_CONFIG_UPDATE) {
        /**
         *  Notify MAC engine to update mac_info.bcn_info
         *  (no matter whether MAC engine has started or not)
         *  \sa ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY
         *  \sa ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED
         *  \sa ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED
         *  \sa ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER
         *  \sa ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR
         */
        os_ipc_set_signal(gl_gateway_mac_engine_pid,
                          SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_INFO);
        return signal ^ SIGNAL_MAC_DRIVER_BCN_CONFIG_UPDATE;
    }

    if (signal & SIGNAL_MAC_DRIVER_CONFIG_BEACON_CLASSES_NUM_UPDATE) {
        os_ipc_set_signal(gl_gateway_mac_engine_pid,
                          SIGNAL_GW_MAC_ENGINE_UPDATE_BCN_CLASSES_NUM);
        return signal ^ SIGNAL_MAC_DRIVER_CONFIG_BEACON_CLASSES_NUM_UPDATE;
    }

    if (signal & SIGNAL_MAC_DRIVER_CONFIG_MODEM_CHANNEL_UPDATE) {
        os_ipc_set_signal(gl_gateway_mac_engine_pid,
                          SIGNAL_GW_MAC_ENGINE_UPDATE_MODEM_CHANNEL);
        return signal ^ SIGNAL_MAC_DRIVER_CONFIG_MODEM_CHANNEL_UPDATE;
    }

    print_log(LOG_ERROR, "unknown signal");
    // discard unkown signals.
    return 0;
}

/**
 * Handle downlink control frames from server->gateway cpu->gateway modem.
 * Frame payload format:
 *  repeated {
 *      [control_code]              \sa enum ecp_gw_c2m_control_code
 *      [optional: control_payload] \sa ecp_gw_c2m_control_payload_len[]
 *  }
 *
 * \return 0 if ok, -1 if failed.
 * \sa _gw_c2m_control_cb()
 */
static os_int8 mac_driver_handle_c2m_control_payload(os_uint8 *control, os_uint16 len)
{
    os_uint16 i = 0; // control code index
    enum ecp_gw_c2m_control_code code;

    os_uint8 _beacon_class_num;
    enum _gateway_occupied_capacity _capacity;
    os_uint8 _channel;
    os_boolean _is_join_allowed;
    os_boolean _is_server_connected;
    os_uint8 _max_tx_power;
    os_uint16 _gw_cluster_addr;
    struct ecp_gw_c2m_control_init_info *init_info;

    while (i < len) {
        code = (enum ecp_gw_c2m_control_code) control[i];
        switch (code) {
        case ECP_GW_C2M_GET_MODEM_UUID:
            // should be a single control code! no other control codes.
            if (i == 0 && len == (1 + ecp_gw_c2m_control_payload_len[code])) {
                ecp_gw_modem_m2c_send_control(ECP_GW_M2C_REPORT_MODEM_UUID, NULL, 0);
            } else {
                return -1;
            }
            break;
        case ECP_GW_C2M_INIT_INFO:
            // should be a single control code! no other control codes.
            if (i == 0 && len == (1 + ecp_gw_c2m_control_payload_len[code])) {
                init_info = (void *) & control[i+1];

                _beacon_class_num = init_info->beacon_classes_num;
                _capacity = (enum _gateway_occupied_capacity) init_info->occupied_capacity;
                _channel = init_info->init_modem_channel;
                _is_join_allowed = init_info->is_join_allowed;
                _is_server_connected = init_info->is_server_connected;
                _max_tx_power = init_info->allowable_max_tx_power;
                _gw_cluster_addr = os_ntoh_u16(init_info->gateway_cluster_addr);

                mac_driver_config_set(ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM,
                                      &_beacon_class_num);
                mac_driver_config_set(ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY,
                                      &_capacity);
                mac_driver_config_set(ECP_GW_C2M_UPDATE_MODEM_CHANNEL,
                                      &_channel);
                mac_driver_config_set(ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED,
                                      &_is_join_allowed);
                mac_driver_config_set(ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED,
                                      &_is_server_connected);
                mac_driver_config_set(ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER,
                                      &_max_tx_power);
                mac_driver_config_set(ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR,
                                      &_gw_cluster_addr);

                os_ipc_set_signal(this->_pid, SIGNAL_MAC_DRIVER_START_ENGINE);
            } else {
                return -1;
            }
            break;
        case ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM:
            _beacon_class_num = control[i+1];
            mac_driver_config_set(code, &_beacon_class_num);
            break;
        case ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY:
            _capacity = (enum _gateway_occupied_capacity) control[i+1];
            mac_driver_config_set(code, &_capacity);
            break;
        case ECP_GW_C2M_UPDATE_MODEM_CHANNEL:
            _channel = control[i+1];
            mac_driver_config_set(code, &_channel);
            break;
        case ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED:
            _is_join_allowed = control[i+1];
            mac_driver_config_set(code, &_is_join_allowed);
            break;
        case ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED:
            _is_server_connected = control[i+1];
            mac_driver_config_set(code, &_is_server_connected);
            break;
        case ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER:
            _max_tx_power = control[i+1];
            mac_driver_config_set(code, &_max_tx_power);
            break;
        case ECP_GW_C2M_UPDATE_NEARBY_CHANNELS:
            // todo
            break;
        case ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR:
            _gw_cluster_addr = construct_u16_2(control[i+1], control[i+2]);
            mac_driver_config_set(code, &_gw_cluster_addr);
            break;
        default:
            return -1;
        }
        i += 1+ecp_gw_c2m_control_payload_len[code];
    }

    return 0;
}

/**
 * Handle downlink data frames from server->gateway cpu->gateway modem.
 * Frame payload format:
 *  1. JOIN_CONFIRM frame:  \sa lpwan_parse_gw_join_confirmed()
 *      [1B] type                   \sa enum frame_type_gw
 *      [1B] tx_bcn_seq_id
 *      [12B] dest_modem_uuid
 *      [4B] struct gw_join_confirmed
 *           ([1B] confirm_info, [1B] init_seq_id, [2B] distributed_short_addr)
 *
 *  2. downlink message frame:
 *      [1B] type                   \sa enum frame_type_gw
 *      [1B] tx_bcn_seq_id
 *      [2B] dest_modem_short_addr
 *      [1B] seq_id
 *      [1B] len
 *      [@len] msg
 *
 * \return 0 if ok, -1 if failed.
 * \sa _gw_c2m_data_cb()
 */
static os_int8 mac_driver_handle_c2m_data_payload(os_uint8 *data, os_uint16 len)
{
    if (data[0] == FTYPE_GW_JOIN_CONFIRMED) {
        if (len != (1+1+12+4))
            return -1;

        os_int8 tx_bcn_seq_id;
        modem_uuid_t dest_uuid;
        struct gw_join_confirmed join_response;

        tx_bcn_seq_id = (os_int8) data[1];
        haddock_memcpy(dest_uuid.addr, & data[2], sizeof(modem_uuid_t));
        haddock_memcpy(& join_response, & data[14], sizeof(struct gw_join_confirmed));

        // we don't care the return value of @gw_mac_prepare_tx_join_confirm()
        gw_mac_prepare_tx_join_confirm(& dest_uuid, & join_response, tx_bcn_seq_id);

        return 0;
    }

    if (data[0] == FTYPE_GW_MSG) {
        return 0;
    }

    return -1;
}

void mac_driver_config_set(enum ecp_gw_c2m_control_code item, void *config_value)
{
    // modify configuration
    switch (item) {
    case ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY     :
        haddock_assert(*((os_uint8 *) config_value) < _gateway_occupied_capacity_invalid);
        mac_engine_driver_config.occupied_capacity = \
                *((enum _gateway_occupied_capacity *) config_value);
        break;
    case ECP_GW_C2M_UPDATE_MODEM_CHANNEL         :
        haddock_assert(*((os_uint8 *) config_value) < LPWAN_MAX_RADIO_CHANNELS_NUM);
        mac_engine_driver_config.modem_channel = *((os_uint8 *) config_value);
        break;
    case ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM    :
        haddock_assert(*((os_uint8 *) config_value) < BEACON_MAX_CLASSES_NUM);
        mac_engine_driver_config.beacon_classes_num = *((os_uint8 *) config_value);
        break;
    case ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED       :
        haddock_assert(is_boolean(*((os_boolean *) config_value)));
        mac_engine_driver_config.is_join_allowed = *((os_boolean *) config_value);
        break;
    case ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED   :
        haddock_assert(is_boolean(*((os_boolean *) config_value)));
        mac_engine_driver_config.is_server_connected = *((os_boolean *) config_value);
        break;
    case ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER:
        haddock_assert(*((os_uint8 *) config_value) < RADIO_TX_POWER_LEVELS_NUM);
        mac_engine_driver_config.allowable_max_tx_power = *((os_uint8 *) config_value);
        break;
    case ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR:
        // the gateway's short address cannot be a broadcast value.
        if (*((os_uint16 *) config_value) != 0xFFFF) {
            mac_engine_driver_config.gateway_cluster_addr = *((os_uint16 *) config_value);
        }
        break;
    default:
        __should_never_fall_here();
        break;
    }

    // send signals
    switch (item) {
    case ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY     :
    case ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED       :
    case ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED   :
    case ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER:
    case ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR  :
        os_ipc_set_signal(this->_pid, SIGNAL_MAC_DRIVER_BCN_CONFIG_UPDATE);
        break;
    case ECP_GW_C2M_UPDATE_MODEM_CHANNEL         :
        os_ipc_set_signal(this->_pid, SIGNAL_MAC_DRIVER_CONFIG_MODEM_CHANNEL_UPDATE);
        break;
    case ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM    :
        os_ipc_set_signal(this->_pid, SIGNAL_MAC_DRIVER_CONFIG_BEACON_CLASSES_NUM_UPDATE);
        break;
    default:
        __should_never_fall_here();
        break;
    }
}

void mac_driver_config_get(enum ecp_gw_c2m_control_code item, void *config_value)
{
    switch (item) {
    case ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY     :
        *((enum _gateway_occupied_capacity *) config_value) = \
                        mac_engine_driver_config.occupied_capacity;
        break;
    case ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED       :
        *((os_boolean *) config_value) = mac_engine_driver_config.is_join_allowed;
        break;
    case ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED   :
        *((os_boolean *) config_value) = mac_engine_driver_config.is_server_connected;
        break;
    case ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER:
        *((os_uint8 *) config_value) = mac_engine_driver_config.allowable_max_tx_power;
        break;
    case ECP_GW_C2M_UPDATE_MODEM_CHANNEL         :
        *((os_uint8 *) config_value) = mac_engine_driver_config.modem_channel;
        break;
    case ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM    :
        *((os_uint8 *) config_value) = mac_engine_driver_config.beacon_classes_num;
        break;
    case ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR:
        *((os_uint16 *) config_value) = mac_engine_driver_config.gateway_cluster_addr;
        break;
    default:
        __should_never_fall_here();
        break;
    }
}

const modem_uuid_t *mac_driver_get_modem_uuid(void)
{
    return gw_mac_info_get_modem_uuid();
}
