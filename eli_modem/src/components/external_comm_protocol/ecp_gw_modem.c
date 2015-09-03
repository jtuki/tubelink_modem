/**
 * ecp_gw_modem.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lib/assert.h"
#include "lib/crc_generator.h"
#include "kernel/hdk_memory.h"

#include "app_hostif.h"

#include "ecp_gw_modem.h"
#include "gateway/mac_engine_driver.h"

/**
 * Control frame's control payload length (in bytes).
 * Corresponding to enum ecp_gw_c2m_control_code
 * \sa enum ecp_gw_c2m_control_code
 *
 * 0: no payload
 */
const os_uint8 ecp_gw_c2m_control_payload_len[__ecp_gw_c2m_control_code_max] = {
    0,  /**< get modem uuid */
    sizeof(struct ecp_gw_c2m_control_init_info),  /**< init info */
    1,  /**< update beacon classes num */
    1,  /**< update occupied capacity */
    1,  /**< update modem channel */
    1,  /**< update is join allowed */
    1,  /**< update is server connected */
    1,  /**< update allowable max tx power */
    5,  /**< update nearby channels \sa LPWAN_MAX_RADIO_CHANNELS_NUM */
    2,  /**< update gateway cluster addr \sa ECP_GW_CPU_UPDATE_GATEWAY_CLUSTER_ADDR */
};

/**
 * \sa ecp_gw_modem_m2c_control_code
 * 0: no payload
 */
const os_uint8 ecp_gw_m2c_control_payload_len[__ecp_gw_m2c_control_code_max] = {
    0,  /**< report gateway modem's uuid
           \ref the @src_modem_uuid in @ecp_gw_m2c_frame_header */
    1,  /**< report modem's beacon seq id */
};

/**
 * \ref MAC engine driver's control and data handler.
 * \sa ecp_gw_modem_register_control_callback()
 * \sa ecp_gw_modem_register_data_callback()
 */
static ecp_gw_modem_c2m_control_callback_fn _gw_c2m_control_cb;
static ecp_gw_modem_c2m_data_callback_fn    _gw_c2m_data_cb;

/**
 * \sa ecp_gw_modem_m2c_send_control()
 * \sa ecp_gw_modem_m2c_send_data()
 * \sa ecp_gw_modem_init() will init this tx buffer
 */
static os_uint8 ecp_gw_m2c_tx_buf[HOSTIF_UAR_BUF_DEFAULT_SIZE];
static os_uint8 ecp_gw_m2c_tx_frame_seq_id = 0;    /**< global tx frame seq id */

static os_boolean ecp_gw_modem_inited = OS_FALSE;   /**< \sa ecp_gw_modem_init() */

/*---------------------------------------------------------------------------*/
/**< @{ static function declarations */
static inline os_uint8 ecp_gw_m2c_increment_frame_seq_id(void);
static os_uint8 ecp_gw_c2m_is_valid_frame(os_uint8 *recv_frame_buf, os_uint16 len);
/**< @} */
/*---------------------------------------------------------------------------*/

void ecp_gw_modem_init(void)
{
    struct ecp_gw_m2c_frame_header *hdr = (void *) ecp_gw_m2c_tx_buf;
    hdr->constant_header[0] = 0xAA;
    hdr->constant_header[1] = 0x55;
    hdr->frame_seq_id = ecp_gw_m2c_tx_frame_seq_id;


    hdr->src_modem_uuid = * mac_driver_get_modem_uuid(); // struct assignment

    ecp_gw_modem_inited = OS_TRUE;
}

os_boolean is_ecp_gw_modem_inited(void)
{
    return ecp_gw_modem_inited;
}

void ecp_gw_modem_register_c2m_control_callback(ecp_gw_modem_c2m_control_callback_fn cb)
{
    if (cb)
        _gw_c2m_control_cb = cb;
}

void ecp_gw_modem_register_c2m_data_callback(ecp_gw_modem_c2m_data_callback_fn cb)
{
    if (cb)
        _gw_c2m_data_cb = cb;
}

/**
 * Callback handler/dispatcher for commands/data from gateway's host CPU.
 * @return Return 0 if successfully parse and dispatch. -1 otherwise.
 *
 * \note We don't handle ACK currently. And also, we don't generate ACK frames.
 */
os_int8 ecp_gw_modem_dispatcher(os_uint8 *recv_frame_buf, os_uint16 len)
{
    os_uint8 payload_len = ecp_gw_c2m_is_valid_frame(recv_frame_buf, len);
    if (payload_len == 0)
        return -1;

    struct ecp_gw_c2m_frame_header *hdr = (struct ecp_gw_c2m_frame_header *) recv_frame_buf;

    switch ((enum ecp_frame_type) hdr->frame_type) {
        case ECP_FTYPE_CONTROL:
            if (_gw_c2m_control_cb) {
                _gw_c2m_control_cb(hdr->payload, (os_uint16) payload_len);
            }
            // we don't generate ACK frames todo
            break;
        case ECP_FTYPE_DATA:
            if (_gw_c2m_data_cb) {
                _gw_c2m_data_cb(hdr->payload, (os_uint16) payload_len);
            }
            // we don't generate ACK frames todo
            break;
        case ECP_FTYPE_ACK:
            // we don't handle it currently. todo
            // note that as for ACK frames, @paylaod_len / 2 == @ack_num
            break;
        default:
            __should_never_fall_here();
            break;
        }

    return 0;
}

static os_uint8 ecp_gw_c2m_is_valid_frame(os_uint8 *recv_frame_buf, os_uint16 len)
{
    struct ecp_gw_c2m_frame_header *hdr = (struct ecp_gw_c2m_frame_header *) recv_frame_buf;

    // validate the @constant_header
    if (! (hdr->constant_header[0] == 0xAA && hdr->constant_header[1] == 0x55))
        return 0;

    // validate the dest_modem_uuid
    if (! (lpwan_uuid_is_equal(mac_driver_get_modem_uuid(), & hdr->dest_modem_uuid)
          || lpwan_uuid_is_broadcast(& hdr->dest_modem_uuid)))
        return 0;

    // validate the frame length and 16-bits CRC.
    os_uint8 frame_len = offsetof(struct ecp_gw_c2m_frame_header, payload);
    os_uint8 payload_len = 0;
    if (len < frame_len)
        return 0;

    switch ((enum ecp_frame_type) hdr->frame_type) {
    case ECP_FTYPE_CONTROL:
    case ECP_FTYPE_DATA:
        payload_len = hdr->len_or_num.payload_len;
        frame_len += payload_len;
        break;
    case ECP_FTYPE_ACK:
        payload_len = sizeof(os_uint8) * hdr->len_or_num.ack_num;
        frame_len += payload_len;
        break;
    default:
        // wrong frame
        return 0;
    }

    // validate frame length
    if (len != frame_len + 2) // 2 is for 2-byte CRC
        return 0;

    // validate the 16-bits CRC
    os_uint16 _crc16;
    _crc16 = os_hton_u16(crc16_generator(recv_frame_buf, frame_len));
    // Note that construct_u16_2 using network-endian (big-endian)
    if (_crc16 != construct_u16_2(recv_frame_buf[frame_len], recv_frame_buf[frame_len+1]))
        return 0;

    // valid frame if @payload_len > 0!
    return payload_len;
}

static inline os_uint8 ecp_gw_m2c_increment_frame_seq_id(void)
{
    // we don't handle ACK currently.
    return ++ecp_gw_m2c_tx_frame_seq_id;
}

/**
 * \sa ecp_gw_m2c_tx_buf[]
 *
 * \note @data might be NULL and @len might be 0.
 * \sa ecp_gw_m2c_control_payload_len
 */
void ecp_gw_modem_m2c_send_control(enum ecp_gw_m2c_control_code code,
                                   void *data, os_uint16 len)
{
    os_uint8 tx_len = 0;
    os_uint16 crc16;
    struct ecp_gw_m2c_frame_header *hdr = (void *) ecp_gw_m2c_tx_buf;

    haddock_assert(is_ecp_gw_modem_inited());

    switch (code) {
    case ECP_GW_M2C_REPORT_MODEM_UUID:
        haddock_assert(data == NULL
                       && len == ecp_gw_m2c_control_payload_len[ECP_GW_M2C_REPORT_MODEM_UUID]);
        tx_len += sizeof(struct ecp_gw_m2c_frame_header);

        hdr->frame_seq_id = ecp_gw_m2c_increment_frame_seq_id();
        hdr->frame_type = ECP_FTYPE_CONTROL;

        hdr->len_or_num.payload_len = 1 + ecp_gw_m2c_control_payload_len[ECP_GW_M2C_REPORT_MODEM_UUID];
        ecp_gw_m2c_tx_buf[tx_len+1] = (os_uint8) ECP_GW_M2C_REPORT_MODEM_UUID;

        tx_len += hdr->len_or_num.payload_len;

        crc16 = crc16_generator(ecp_gw_m2c_tx_buf, tx_len);
        // decompose the crc16 using network-endian (big-endian)
        decompose_u16_2(crc16, &ecp_gw_m2c_tx_buf[tx_len], &ecp_gw_m2c_tx_buf[tx_len+1]);

        tx_len += 2; // crc part
        break;
    case ECP_GW_M2C_REPORT_BEACON_SEQ:
        // todo
        break;
    default:
        // wrong code
        return;
    }

    if (tx_len > 0) {
        hostIf_SendToHost((hostIfChar *) ecp_gw_m2c_tx_buf, tx_len);
    }
}

/**
 * \sa ecp_gw_m2c_tx_buf[]
 *
 */
void ecp_gw_modem_m2c_send_data(void *data, os_uint16 len,
                                os_int16 snr, os_int16 rssi)
{
    os_uint16 tx_len = 0;
    os_uint16 crc16;

    os_int16 _snr   = os_hton_u16((os_uint16) snr);
    os_int16 _rssi  = os_hton_u16((os_uint16) rssi);

    struct ecp_gw_m2c_frame_header *hdr = (void *) ecp_gw_m2c_tx_buf;

    haddock_assert(is_ecp_gw_modem_inited());
    haddock_assert(data != NULL && len > 0);

    tx_len += sizeof(struct ecp_gw_m2c_frame_header);

    hdr->frame_seq_id = ecp_gw_m2c_increment_frame_seq_id();
    hdr->frame_type = ECP_FTYPE_DATA;

    hdr->len_or_num.payload_len = 4 + len;
    haddock_memcpy(hdr->payload, &_snr, 2);
    haddock_memcpy(hdr->payload + 2, &_rssi, 2);
    haddock_memcpy(hdr->payload + 4, data, len);

    tx_len += hdr->len_or_num.payload_len;

    crc16 = crc16_generator(ecp_gw_m2c_tx_buf, tx_len);
    decompose_u16_2(crc16, &ecp_gw_m2c_tx_buf[tx_len], &ecp_gw_m2c_tx_buf[tx_len+1]);

    tx_len += 2; // crc part

    if (tx_len > 0) {
        hostIf_SendToHost((hostIfChar *) ecp_gw_m2c_tx_buf, tx_len);
    }
}
