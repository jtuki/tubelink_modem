/**
 * packed_ack_frames_mgmt.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PACKED_ACK_FRAMES_MGMT_H_
#define PACKED_ACK_FRAMES_MGMT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lib/linked_list.h"

#include "lpwan_types.h"
#include "frame_defs/common_defs.h"
#include "frame_defs/gw_beacon.h"

#include "radio_config.h"

#include "protocol_utils/parsed_common_defs.h"

struct gw_downlink_fbuf {
    struct list_head hdr;
    struct lpwan_addr dest;
    enum frame_type_gw type;
    os_int8 expected_downlink_list_id;  /**< \sa packed_ack_expected_list_id() */
    os_uint8 len;
    os_uint8 frame[LPWAN_RADIO_TX_BUFFER_MAX_LEN] __attribute__((aligned (4)));
};

/*---------------------------------------------------------------------------*/
/** packed ACK */

void packed_ack_mgmt_alloc_buffer(void);

void packed_ack_mgmt_init_list_id(os_int8 id);
void packed_ack_mgmt_reset_buffer(void);

void packed_ack_ticktock(void);

os_int8 packed_ack_expected_list_id(os_int8 packed_ack_delay_num);

os_uint8 packed_ack_cur_list_len(void);
os_int8 packed_ack_push(const struct beacon_packed_ack *ack);
struct beacon_packed_ack *packed_ack_pop(struct beacon_packed_ack *ack);

os_boolean has_packed_ack(void);
/*---------------------------------------------------------------------------*/
/** Downlink frames. */

void downlink_frames_mgmt_init(void);
void downlink_frames_mgmt_reset(void);

void downlink_frame_clear_cur_list(void);
void downlink_frame_clear_tx_list(void);

os_uint8 downlink_frames_cur_list_len(void);
os_uint8 downlink_frames_tx_list_len(void);

const struct gw_downlink_fbuf *downlink_frame_alloc(void);
void downlink_frame_free(const struct gw_downlink_fbuf *fbuf);

os_int8 downlink_frame_put(const struct gw_downlink_fbuf *c_fbuf,
                           enum frame_type_gw type,
                           const struct lpwan_addr *dest,
                           const os_uint8 frame[], os_uint8 len,
                           os_int8 bcn_seq_id, os_int8 expected_bcn_seq_id);

os_boolean downlink_frames_check(os_boolean is_join_ack, modem_short_t *addr);

const struct gw_downlink_fbuf *downlink_frames_tx_list_get_next(void);
void downlink_frames_handle_tx_ok(const struct gw_downlink_fbuf *fbuf);
void downlink_frames_handle_tx_failed(const struct gw_downlink_fbuf *fbuf);

/*---------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* PACKED_ACK_FRAMES_MGMT_H_ */
