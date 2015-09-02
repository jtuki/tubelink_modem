/**
 * ecp_gw_modem.h
 *
 *  The external communication protocol between gateway modem and CPU.
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef ECP_GW_MODEM_H_
#define ECP_GW_MODEM_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "frame_defs/common_defs.h"

#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(push)
#pragma pack(1)
#endif

/**
 * The common frame header formats sent from gateway CPU to gateway modem.
 * \sa ecp_gw_m2c_frame_header
 */
struct ecp_gw_c2m_frame_header {
    os_uint8 constant_header[2];    /**< 0xAA 0x55 */
    modem_uuid_t dest_modem_uuid;
    os_uint8 frame_seq_id;
    os_uint8 frame_type;        /**< \sa enum ecp_frame_type */
    union {
        os_uint8 payload_len;  /**< for control and data frames */
        os_uint8 ack_num;      /**< for ack frame */
    } len_or_num;
    os_uint8 payload[];
};

/**
 * The common frame header formats sent from gateway modem to gateway CPU.
 * \sa ecp_gw_c2m_frame_header
 *
 * \note As for data frames received from end-devices:
 *  @payload's first 4 bytes are the RSSI and SNR value: [os_int16: SNR][os_int16: RSSI].
 *  @len_or_num.payload_len contain this 4 bytes.
 * \sa ecp_gw_modem_m2c_send_data(buf, len, snr, rssi)
 */
struct ecp_gw_m2c_frame_header {
    os_uint8 constant_header[2];    /**< 0xAA 0x55 */
    modem_uuid_t src_modem_uuid;
    os_uint8 frame_seq_id;
    os_uint8 frame_type;        /**< \sa enum ecp_frame_type */
    union {
        os_uint8 payload_len;  /**< for control and data frames */
        os_uint8 ack_num;      /**< for ack frame */
    } len_or_num;
    os_uint8 payload[];
};

/**< Init modem control frame's payload length:
         beacon_classes_num;
         occupied_capacity;
         init_modem_channel;
         is_join_allowed;
         is_server_connected;
         allowable_max_tx_power;
         gateway_cluster_addr;
    */
struct ecp_gw_c2m_control_init_info {
    os_uint8    beacon_classes_num;
    os_uint8    occupied_capacity;
    os_uint8    init_modem_channel;
    os_boolean  is_join_allowed;
    os_boolean  is_server_connected;
    os_uint8    allowable_max_tx_power;
    os_uint16   gateway_cluster_addr;
};

enum ecp_frame_type {
    ECP_FTYPE_CONTROL = 0,
    ECP_FTYPE_DATA,
    ECP_FTYPE_ACK,      /**< we don't care about ACK currently */
};

/**
 * The control code sent by gateway CPU to gateway modems.
 * \sa ecp_gw_c2m_control_payload_len
 */
enum ecp_gw_c2m_control_code {
    ECP_GW_C2M_GET_MODEM_UUID = 0,
    ECP_GW_C2M_INIT_INFO,   /**< init information for modem initialization */

    /**< @{ for MAC driver's configuration update
     * \sa mac_driver_config_set()
     * \sa mac_driver_config_get() */
    ECP_GW_C2M_UPDATE_BEACON_CLASSES_NUM,
    ECP_GW_C2M_UPDATE_OCCUPIED_CAPACITY,
    ECP_GW_C2M_UPDATE_MODEM_CHANNEL,
    ECP_GW_C2M_UPDATE_IS_JOIN_ALLOWED,
    ECP_GW_C2M_UPDATE_IS_SERVER_CONNECTED,
    ECP_GW_C2M_UPDATE_ALLOWABLE_MAX_TX_POWER,
    ECP_GW_C2M_UPDATE_NEARBY_CHANNELS,  /**< bit-vector nearby channel information.
                                           \sa LPWAN_MAX_RADIO_CHANNELS_NUM */
    ECP_GW_C2M_UPDATE_GATEWAY_CLUSTER_ADDR,
    /**< @} */

    __ecp_gw_c2m_control_code_max,
};

enum ecp_gw_m2c_control_code {
    ECP_GW_M2C_REPORT_MODEM_UUID = 0,
    ECP_GW_M2C_REPORT_BEACON_SEQ,
    __ecp_gw_m2c_control_code_max,
};

extern const os_uint8 ecp_gw_c2m_control_payload_len[__ecp_gw_c2m_control_code_max];
extern const os_uint8 ecp_gw_m2c_control_payload_len[__ecp_gw_m2c_control_code_max];

typedef os_int8 (*ecp_gw_modem_c2m_control_callback_fn)(os_uint8 *control_payload, os_uint16 len);
typedef os_int8 (*ecp_gw_modem_c2m_data_callback_fn)(os_uint8 *data_payload, os_uint16 len);

void ecp_gw_modem_init(void);
os_boolean is_ecp_gw_modem_inited(void);

void ecp_gw_modem_register_c2m_control_callback(ecp_gw_modem_c2m_control_callback_fn cb);
void ecp_gw_modem_register_c2m_data_callback(ecp_gw_modem_c2m_data_callback_fn cb);

/**
 * Send control information, from gateway modem to gateway CPU.
 */
void ecp_gw_modem_m2c_send_control(enum ecp_gw_m2c_control_code code,
                                   void *data, os_uint16 len);
/**
 * Send data information, from gateway modem to gateway CPU.
 */
void ecp_gw_modem_m2c_send_data(void *data, os_uint16 len,
                                os_int16 snr, os_int16 rssi);

/**
 * Dispatch received frame (sent from gateway CPU) to registered handler.
 */
os_int8 ecp_gw_modem_dispatcher(os_uint8 *recv_frame_buf, os_uint16 len);


#if defined(__CC_ARM) || defined(__GNUC__)
#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ECP_GW_MODEM_H_ */
