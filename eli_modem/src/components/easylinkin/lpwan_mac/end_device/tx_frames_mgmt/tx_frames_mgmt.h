/**
 * tx_frames_mgmt.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef TX_FRAMES_MGMT_H_
#define TX_FRAMES_MGMT_H_

/**
 * Manage the end-deivce's MAC engine's tx frames:
 * 1) pending tx frames
 * 2) wait for ACK frames
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include "haddock_types.h"
#include "frame_defs/common_defs.h"
#include "end_device/mac_engine_config.h"

/**
 * TX frame buffer for device's uplink messages.
 * Store the transmit times, beacon seq id (tx id, expected rx id), msg type/content etc.
 */
struct tx_fbuf {
    struct list_head hdr;

    os_uint8 transmit_times;
    os_uint8 tx_fail_times;         /**< try to tx but failed due to backoff collision etc.
                                         \note if tx failed, we don't count @transmit_times.
                                         \sa mac_tx_frames_handle_tx_msg_failed() */

    os_uint8 seq;                   /**< tx frame's seq */
    os_int8 beacon_seq_id;          /**< the beacon sequence id when the message is sent. */
    os_int8 expected_beacon_seq_id; /**< the expected beacon seq id to received ACK. */

    enum frame_type_end_device ftype;
    enum device_message_type msg_type;  /**< if @ftype is FTYPE_DEVICE_MSG, use
                                           this @msg_type to decide its type. */
    os_uint8 len;
    os_uint8 frame[LPWAN_DEVICE_MAC_UPLINK_MTU] __attribute__((aligned (4)));
};

void mac_tx_frames_mgmt_init(void);

void mac_tx_frames_mgmt_handle_join_ok(void);

os_int8 mac_tx_frames_put_msg(enum device_message_type type,
                              const os_uint8 msg[], os_uint8 len,
                              short_addr_t src, short_addr_t dest);
const struct tx_fbuf *mac_tx_frames_get_msg(void);

void mac_tx_frames_prepare_tx_msg(const struct tx_fbuf *c_fbuf,
                                  os_int8 bcn_seq_id, os_int8 expected_bcn_seq_id, os_uint8 bcn_class_seq_id,
                                  os_int16 bcn_rssi, os_int16 bcn_snr);
void mac_tx_frames_handle_tx_msg_ok(const struct tx_fbuf *c_fbuf);
void mac_tx_frames_handle_tx_msg_failed(const struct tx_fbuf *c_fbuf);

void mac_tx_frames_handle_check_ack(os_int8 bcn_seq_id, os_uint8 confirm_seq);
void mac_tx_frames_handle_no_ack(os_int8 bcn_seq_id);

void mac_tx_frames_handle_lost_beacon(void);

/** \return TRUE if has pending tx or JOIN_REQ. FALSE otherwise.*/
os_boolean mac_tx_frames_has_pending_tx(void);
/** \return TRUE if has waiting ACK or JOIN_CONFIRM. FALSE otherwise. */
os_boolean mac_tx_frames_has_waiting_ack(os_int8 expected_bcn_seq_id);

os_boolean mac_tx_frames_has_pending_join_req(void);
os_boolean mac_tx_frames_has_waiting_join_confirm(os_int8 expected_bcn_seq_id);

#ifdef __cplusplus
}
#endif

#endif /* TX_FRAMES_MGMT_H_ */
