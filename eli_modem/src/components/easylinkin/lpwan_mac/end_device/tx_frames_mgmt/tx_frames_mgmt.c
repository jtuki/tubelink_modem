/**
 * tx_frames_mgmt.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef TX_FRAMES_MGMT_C_
#define TX_FRAMES_MGMT_C_

#ifdef __cplusplus
extern "C"
{
#endif

#include "lib/mem_pool.h"
#include "lib/linked_list.h"
#include "lib/assert.h"

#include "lpwan_config.h"
#include "lpwan_utils.h"

#include "protocol_utils/construct_frame_hdr.h"
#include "protocol_utils/construct_de_uplink_common.h"
#include "protocol_utils/construct_de_uplink_msg.h"
#include "protocol_utils/parse_beacon.h"
#include "protocol_utils/construct_de_join_req.h"

#include "end_device/mac_engine.h"
#include "tx_frames_mgmt.h"

/**
 * \sa LPWAN_DEVICE_MAC_TX_FBUF_MAX_NUM
 * \sa sizeof(struct tx_frame_buffer)
 */
static struct mem_pool_hdr *gl_uplink_frame_pool;

static struct list_head gl_wait_ack_frame_buffer_list;     /**< frames awaiting ACK */
static struct list_head gl_pending_tx_frame_buffer_list;   /**< frames (uplink messages) pending to tx */

static os_uint8 gl_wait_ack_list_len;     /**< \sa LPWAN_DEVICE_MAC_TX_FBUF_MAX_NUM */
static os_uint8 gl_pending_tx_list_len;      /**< \sa LPWAN_DEVICE_MAC_TX_FBUF_MAX_NUM */

static os_uint8 gl_tx_frame_seq_id; /**< init to a random value after join network,
                                         increment 1 for each newly tx frame */

static void mac_tx_fbuf_init_msg(struct tx_fbuf *fbuf,
                                 enum device_message_type type,
                                 short_addr_t src, short_addr_t dest,
                                 const os_uint8 msg[], os_uint8 len);

static void mac_tx_frames_mgmt_init_frame_seq_id(void);
static os_boolean mac_tx_fbuf_is_within_pending_tx_list(const struct tx_fbuf *fbuf);

static struct tx_fbuf *mac_tx_fbuf_alloc(void);
static void mac_tx_fbuf_free(const struct tx_fbuf *buffer);

/**
 * If @has_join_req, the MAC engine tries to send JOIN_REQ according to @fbuf.
 */
static struct {
    os_boolean has_join_req;    /**< JOIN_REQ available */
    os_boolean has_been_tx;     /**< JOIN_REQ has been sent, is awaiting ACK */
    struct tx_fbuf fbuf;
} gl_tx_join_req;

void mac_tx_frames_mgmt_init(void)
{
    gl_uplink_frame_pool = mem_pool_create(LPWAN_DEVICE_MAC_TX_FBUF_MAX_NUM,
                                           sizeof(struct tx_fbuf));
    haddock_assert(gl_uplink_frame_pool);

    list_head_init(& gl_wait_ack_frame_buffer_list);
    list_head_init(& gl_pending_tx_frame_buffer_list);

    gl_wait_ack_list_len = 0;
    gl_pending_tx_list_len = 0;
}

/**
 * Init the global tx frame seq ID.
 * \sa struct device_join_request::init_seq_id
 */
static void mac_tx_frames_mgmt_init_frame_seq_id(void)
{
    static os_boolean run_first_time = OS_TRUE;
    if (run_first_time) {
        gl_tx_frame_seq_id = hdk_randr(0, 255);
        run_first_time = OS_FALSE;
    } else {
        // not the first time joining
        gl_tx_frame_seq_id += 1;
    }
}

/**
 * Callback function of joined network (mac engine has successfully joined network).
 */
void mac_tx_frames_handle_join_ok(short_addr_t my_addr, short_addr_t gw_addr)
{
    haddock_assert(gl_wait_ack_list_len == 0);

    static struct lpwan_addr f_src;   // change to a new short addr
    static struct lpwan_addr f_dest;  // change to a new gateway cluster addr

    f_dest.type = ADDR_TYPE_SHORT_ADDRESS;
    f_src.type = ADDR_TYPE_SHORT_ADDRESS;
    f_dest.addr.short_addr = gw_addr;
    f_src.addr.short_addr = my_addr;

    struct list_head *pos;
    struct tx_fbuf *fbuf;

    list_for_each(pos, & gl_pending_tx_frame_buffer_list) {
        fbuf = list_entry(pos, struct tx_fbuf, hdr);

        fbuf->transmit_times = 0;
        fbuf->tx_fail_times = 0;

        update_device_frame_header_addr((struct frame_header *) fbuf->frame, FRAME_HDR_LEN_NORMAL,
                                        FTYPE_DEVICE_MSG, & f_src, & f_dest);
    }

    haddock_assert(gl_wait_ack_list_len == 0); /* \sa mac_tx_frames_handle_lost_beacon() */

    gl_tx_join_req.has_join_req = OS_FALSE;
    gl_tx_join_req.has_been_tx = OS_FALSE;
}

/**
 * Try to put a message into the tx message frame buffer.
 *
 * \note All the src and dest _should_ be changed if the end-device has changed
 *       its parent gateway (after a new JOIN, has a new source short_addr etc.).
 *       \sa mac_tx_frames_handle_join_ok()
 *
 * \return -1 if frame buffer is full, frame's total length otherwise (successfully
 * put the message into the tx frame buffer).
 */
os_int8 mac_tx_frames_put_msg(enum device_message_type type,
                              const os_uint8 msg[], os_uint8 len,
                              short_addr_t src, short_addr_t dest)
{
    haddock_assert(msg && len <= LPWAN_MAX_PAYLAOD_LEN);

    struct tx_fbuf *fbuf = mac_tx_fbuf_alloc();
    if (! fbuf) {
        return -1;
    }

    // init the message buffer
    mac_tx_fbuf_init_msg(fbuf, type, src, dest, msg, len);

    // linked newly allocated frame buffer to pending tx list (add to tail).
    list_add_tail(& fbuf->hdr, & gl_pending_tx_frame_buffer_list);
    gl_pending_tx_list_len += 1;

    return (os_int8) fbuf->len;
}

/**
 * We don't allow others to modify the tx frame buffer outside this module,
 * so return a _const_ struct tx_buf pointer.
 */
const struct tx_fbuf *mac_tx_frames_get_msg(void)
{
    struct tx_fbuf *fbuf;
    if (gl_pending_tx_list_len > 0) {
        haddock_assert(! list_empty(& gl_pending_tx_frame_buffer_list));

        struct list_head *pos;
        pos = gl_pending_tx_frame_buffer_list.next;
        fbuf = list_entry(pos, struct tx_fbuf, hdr);
        return fbuf;
    } else {
        return NULL;
    }
}

/**
 * See if @fbuf is linked within gl_pending_tx_frame_buffer_list.
 */
static os_boolean mac_tx_fbuf_is_within_pending_tx_list(const struct tx_fbuf *fbuf)
{
    haddock_assert(fbuf && !list_empty(& fbuf->hdr));

    os_boolean is_within = OS_FALSE;

    if (gl_pending_tx_list_len > 0) {
        haddock_assert(! list_empty(& gl_pending_tx_frame_buffer_list));

        os_uint8 i = 0;
        const struct list_head *pos = fbuf->hdr.next;
        for (; i < LPWAN_DEVICE_MAC_TX_FBUF_MAX_NUM; pos = pos->next) {
            if (pos == & gl_pending_tx_frame_buffer_list) {
                is_within = OS_TRUE;
                break;
            }
        }
    }
    return is_within;
}

/**
 * Get prepared to tx @c_fbuf.
 *
 * \note Other modules can only get a const struct tx_buf pointer, so we will cast
 *       a const pointer @c_fbuf to non-const pointer @fbuf in the function.
 * \sa mac_tx_frames_get_msg()
 */
void mac_tx_frames_prepare_tx_msg(const struct tx_fbuf *c_fbuf,
                                  os_int8 bcn_seq_id, os_int8 expected_bcn_seq_id, os_uint8 bcn_class_seq_id,
                                  os_int16 bcn_rssi, os_int16 bcn_snr)
{
    haddock_assert(mac_tx_fbuf_is_within_pending_tx_list(c_fbuf));
    struct tx_fbuf *fbuf = (struct tx_fbuf *)c_fbuf;

    fbuf->beacon_seq_id = bcn_seq_id;
    fbuf->expected_beacon_seq_id = expected_bcn_seq_id;

    if (fbuf->transmit_times == 0) {
        // a newly tx frame
        gl_tx_frame_seq_id += 1;
        fbuf->seq = gl_tx_frame_seq_id;
    }

    fbuf->transmit_times += 1;

    update_device_uplink_msg((struct device_uplink_msg *) & fbuf->frame[FRAME_HDR_LEN_NORMAL],
                             fbuf->seq,
                             fbuf->transmit_times - 1, // retransmit_num = trans_num - 1;
                             fbuf->tx_fail_times,
                             bcn_seq_id, bcn_class_seq_id,
                             bcn_rssi, bcn_snr);
}

/**
 * Transfer @c_fbuf from pending tx list to wait ACK list.
 */
void mac_tx_frames_handle_tx_msg_ok(const struct tx_fbuf *c_fbuf)
{
    haddock_assert(mac_tx_fbuf_is_within_pending_tx_list(c_fbuf));
    struct tx_fbuf *fbuf = (struct tx_fbuf *)c_fbuf;

    list_move_tail(& fbuf->hdr,
                   & gl_wait_ack_frame_buffer_list);
    gl_pending_tx_list_len -= 1;
    gl_wait_ack_list_len += 1;
}

/**
 * Handle tx msg fail condition.
 * \sa struct tx_buf::tx_fail_times
 */
void mac_tx_frames_handle_tx_msg_failed(const struct tx_fbuf *c_fbuf)
{
    // leave @c_fbuf remains in pending tx list.
    haddock_assert(mac_tx_fbuf_is_within_pending_tx_list(c_fbuf));
    struct tx_fbuf *fbuf = (struct tx_fbuf *)c_fbuf;

    fbuf->transmit_times -= 1;  /**< we don't tx the frame, decrement the transmit times */
    fbuf->tx_fail_times += 1;

    if (fbuf->transmit_times >= (LPWAN_DE_MAX_RETRANSMIT_NUM+1)
        || fbuf->tx_fail_times > LPWAN_DE_MAX_TX_FAIL_NUM) {
        // delete the message if too many transmission times.
        list_del_init(& fbuf->hdr);
        mac_tx_fbuf_free(fbuf);

        print_log(LOG_WARNING, "tx_mgmt: del msg(%d) (tx-fail)(tx:%d;tx_f:%d)",
                  fbuf->seq,
                  fbuf->transmit_times, fbuf->tx_fail_times);
    }
}

/**
 * If the MAC engine received the packed ACK for device's uplink message, it will
 * notify tx_frames_mgmt to handle the ACK.
 *
 * \sa gl_wait_ack_frame_buffer_list
 */
void mac_tx_frames_handle_check_ack(os_int8 bcn_seq_id, os_uint8 confirm_seq)
{
    haddock_assert(mac_tx_frames_has_waiting_ack(bcn_seq_id));

    struct list_head *pos, *n;
    struct tx_fbuf *fbuf;

    os_boolean has_no_ack = OS_FALSE;

    list_for_each_safe(pos, n, & gl_wait_ack_frame_buffer_list) {
        fbuf = list_entry(pos, struct tx_fbuf, hdr);

        os_int16 cmp = calc_bcn_seq_delta(fbuf->expected_beacon_seq_id, bcn_seq_id);
        haddock_assert(cmp >= 0);

        if (cmp == 0 && fbuf->seq == confirm_seq) {
            // matched! expect ACK and ACK received.
            list_del_init(pos);
            mac_tx_fbuf_free(fbuf);
            gl_wait_ack_list_len -= 1;

            print_log(LOG_INFO_COOL, "tx_mgmt: ack\t\t<%d", confirm_seq);
        } else if (cmp == 0) {
            // odd condition! (confirm_seq != seq)
            has_no_ack = OS_TRUE;
        }
    }

    if (has_no_ack) {
        mac_tx_frames_handle_no_ack(bcn_seq_id);
    }
}

/**
 * If MAC engine is expecting an ACK, but no ACK received, handle the situation.
 */
void mac_tx_frames_handle_no_ack(os_int8 bcn_seq_id)
{
    haddock_assert(mac_tx_frames_has_waiting_ack(bcn_seq_id));

    struct list_head *pos, *n;
    struct tx_fbuf *fbuf;

    list_for_each_safe(pos, n, & gl_wait_ack_frame_buffer_list) {
        fbuf = list_entry(pos, struct tx_fbuf, hdr);

        os_int16 cmp = calc_bcn_seq_delta(fbuf->expected_beacon_seq_id, bcn_seq_id);
        haddock_assert(cmp >= 0);

        if (cmp == 0) {
            // matched! expect ACK but no ACK received.
            if (fbuf->transmit_times < (LPWAN_DE_MAX_RETRANSMIT_NUM+1)
                && fbuf->tx_fail_times < LPWAN_DE_MAX_TX_FAIL_NUM) {
                // retry tx
                list_move_tail(pos, & gl_pending_tx_frame_buffer_list);
                gl_wait_ack_list_len -= 1;
                gl_pending_tx_list_len += 1;

                print_log(LOG_WARNING, "tx_mgmt: no ack(%d) retry(tx:%d;tx_f:%d)",
                          fbuf->seq,
                          fbuf->transmit_times, fbuf->tx_fail_times);
            } else {
                // too many tx times, discard.
                list_del_init(pos);
                mac_tx_fbuf_free(fbuf);
                gl_wait_ack_list_len -= 1;

                print_log(LOG_WARNING, "tx_mgmt: no ack(%d) discard(tx:%d;tx_f:%d)",
                          fbuf->seq,
                          fbuf->transmit_times, fbuf->tx_fail_times);
            }
        }
    }
}

const struct tx_fbuf *mac_tx_frames_get_join_frame(void)
{
    haddock_assert(gl_tx_join_req.fbuf.ftype == FTYPE_DEVICE_JOIN);
    return & gl_tx_join_req.fbuf;
}

/**
 * \return TRUE if is valid JOIN_REQ ACK, FALSE otherwise.
 * \note suuid is a 2 byte hash of modem_uuid_t, and may conflict with 2 byte short_addr,
 *       if conflict occurs (rare condition), we regard the JOIN_REQ ACK as invalid.
 */
os_boolean mac_tx_frames_handle_join_ack(os_int8 bcn_seq_id, os_uint8 confirm_seq)
{
    haddock_assert(mac_tx_frames_has_waiting_join_confirm(bcn_seq_id));
    gl_tx_join_req.has_been_tx = OS_FALSE;
    return (gl_tx_join_req.fbuf.seq == confirm_seq) ? OS_TRUE : OS_FALSE;
}

void mac_tx_frames_handle_no_join_ack(os_int8 bcn_seq_id)
{
    haddock_assert(mac_tx_frames_has_waiting_join_confirm(bcn_seq_id));
    gl_tx_join_req.has_been_tx = OS_FALSE;
}

void mac_tx_frames_handle_tx_join_ok(void)
{
    haddock_assert(gl_tx_join_req.has_join_req == OS_TRUE
                   && gl_tx_join_req.has_been_tx == OS_FALSE);
    gl_tx_join_req.has_been_tx = OS_TRUE;
}

void mac_tx_frames_handle_tx_join_failed(void)
{
    haddock_assert(gl_tx_join_req.has_join_req == OS_TRUE
                   && gl_tx_join_req.has_been_tx == OS_FALSE);
    gl_tx_join_req.has_been_tx = OS_FALSE; // remain unchanged.
}

/**
 * \remark If lost beacon during beacon tracking, there is no need to wait for
 * gateway's packed ACK.
 */
void mac_tx_frames_handle_lost_beacon(void)
{
    struct list_head *pos;
    struct list_head *n;
    
    enum device_mac_states mac_state = mac_info_get_mac_states();
    switch (mac_state) {
    case DE_MAC_STATES_JOINING:
        // no need to wait JOIN_CONFIRM now. need to tx JOIN_REQ again.
        gl_tx_join_req.has_been_tx = OS_FALSE;
        break;
    case DE_MAC_STATES_JOINED:
        /* Move all the frames from @gl_wait_ack_frame_buffer_list to
         * @gl_pending_tx_frame_buffer_list.
         */
        list_for_each_safe(pos, n, & gl_wait_ack_frame_buffer_list) {
            list_move_tail(pos, & gl_pending_tx_frame_buffer_list);
        }

        gl_pending_tx_list_len += gl_wait_ack_list_len;
        gl_wait_ack_list_len = 0;
        break;
    default:
        __should_never_fall_here();
    }
}

/** \return TRUE if has pending tx or JOIN_REQ. FALSE otherwise.*/
os_boolean mac_tx_frames_has_pending_tx(void)
{
    if (gl_pending_tx_list_len) {
        haddock_assert(! list_empty(& gl_pending_tx_frame_buffer_list));
        return OS_TRUE;
    } else if (mac_tx_frames_has_pending_join_req()) {
        return OS_TRUE;
    } else {
        return OS_FALSE;
    }
}

/** \return TRUE if has waiting ACK or JOIN_CONFIRM. FALSE otherwise. */
os_boolean mac_tx_frames_has_waiting_ack(os_int8 expected_bcn_seq_id)
{
    struct list_head *pos;
    struct tx_fbuf *fbuf;

    os_boolean matched = OS_FALSE;

    if (mac_tx_frames_has_waiting_join_confirm(expected_bcn_seq_id)) {
        matched = OS_TRUE;
    } else {
        list_for_each(pos, & gl_wait_ack_frame_buffer_list) {
            fbuf = list_entry(pos, struct tx_fbuf, hdr);
            if (fbuf->expected_beacon_seq_id == expected_bcn_seq_id) {
                matched = OS_TRUE;
                break;
            }
        }
    }

    return matched;
}

os_boolean mac_tx_frames_has_pending_join_req(void)
{
    if (gl_tx_join_req.has_join_req) {
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINING);
        return OS_TRUE;
    } else {
        return OS_FALSE;
    }
}

os_boolean mac_tx_frames_has_waiting_join_confirm(os_int8 expected_bcn_seq_id)
{
    if (gl_tx_join_req.has_join_req && gl_tx_join_req.has_been_tx
        && gl_tx_join_req.fbuf.expected_beacon_seq_id == expected_bcn_seq_id) {
        haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINING);
        return OS_TRUE;
    } else {
        return OS_FALSE;
    }
}

/**
 * Init the message stored in @fbuf.
 */
static void mac_tx_fbuf_init_msg(struct tx_fbuf *fbuf,
                                 enum device_message_type type,
                                 short_addr_t src, short_addr_t dest,
                                 const os_uint8 msg[], os_uint8 len)
{
    static struct lpwan_addr f_dest;
    static struct lpwan_addr f_src;

    fbuf->ftype = FTYPE_DEVICE_MSG;
    fbuf->msg_type = type;
    fbuf->transmit_times = 0;
    fbuf->tx_fail_times = 0;

    // construct the frame header
    f_dest.type = ADDR_TYPE_SHORT_ADDRESS;
    f_src.type = ADDR_TYPE_SHORT_ADDRESS;
    f_dest.addr.short_addr = dest;
    f_src.addr.short_addr = src;

    construct_device_frame_header(fbuf->frame, LPWAN_DEVICE_MAC_UPLINK_MTU,
                                  FTYPE_DEVICE_MSG,
                                  & f_src, & f_dest,
                                  OS_FALSE, // is_mobile?
                                  RADIO_TX_POWER_LEVELS_NUM-1);

    // construct the uplink message
    construct_device_uplink_msg(type, msg, len,
                                & fbuf->frame[FRAME_HDR_LEN_NORMAL],
                                LPWAN_DEVICE_MAC_UPLINK_MTU - FRAME_HDR_LEN_NORMAL);
    fbuf->len = FRAME_HDR_LEN_NORMAL + sizeof(struct device_uplink_msg) + len;
}

/**
 * Init the JOIN_REQ frame.
 * \sa mac_tx_frames_prepare_join_req()
 */
void mac_tx_frames_init_join_req(void) {
    haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINING);

    static struct lpwan_addr f_dest;
    static struct lpwan_addr f_src;

    // init the frame seq id.
    mac_tx_frames_mgmt_init_frame_seq_id();

    gl_tx_join_req.fbuf.ftype = FTYPE_DEVICE_JOIN;
    gl_tx_join_req.fbuf.transmit_times = 0;
    gl_tx_join_req.fbuf.tx_fail_times = 0;
    gl_tx_join_req.fbuf.seq = gl_tx_frame_seq_id;

    // construct the JOIN_REQ frame header
    f_dest.type = ADDR_TYPE_SHORT_ADDRESS;
    f_src.type = ADDR_TYPE_MODEM_UUID;
    f_dest.addr.short_addr = mac_info_get_short_addr();
    mac_info_get_uuid(& f_src.addr.uuid);

    construct_device_frame_header(gl_tx_join_req.fbuf.frame, LPWAN_DEVICE_MAC_UPLINK_MTU,
                                  FTYPE_DEVICE_JOIN,
                                  & f_src, & f_dest,
                                  OS_FALSE, // is_mobile?
                                  RADIO_TX_POWER_LEVELS_NUM-1);

    // construct the JOIN_REQ body
    construct_device_join_req((void *) & gl_tx_join_req.fbuf.frame[FRAME_HDR_LEN_JOIN],
                              gl_tx_join_req.fbuf.seq, JOIN_REASON_DEFAULT,
                              mac_info_get_app_id());

    // notify the beacon tracker that the JOIN_REQ frame is ready.
    gl_tx_join_req.has_join_req = OS_TRUE;
    gl_tx_join_req.has_been_tx = OS_FALSE;
}

/**
 * \note make sure mac_tx_frames_init_join_req() has been called previously.
 */
void mac_tx_frames_prepare_join_req(const struct tx_fbuf *c_fbuf,
                                    os_int8 bcn_seq_id, os_int8 expected_bcn_seq_id,
                                    os_uint8 bcn_class_seq_id,
                                    os_int16 bcn_rssi, os_int16 bcn_snr)
{
    haddock_assert(c_fbuf == & gl_tx_join_req.fbuf);
    haddock_assert(mac_info_get_mac_states() == DE_MAC_STATES_JOINING
                   && gl_tx_join_req.has_join_req == OS_TRUE
                   && gl_tx_join_req.has_been_tx == OS_FALSE);

    gl_tx_join_req.fbuf.beacon_seq_id = bcn_seq_id;
    gl_tx_join_req.fbuf.expected_beacon_seq_id = expected_bcn_seq_id;

    update_device_join_req((void *) & gl_tx_join_req.fbuf.frame[FRAME_HDR_LEN_JOIN],
                           bcn_seq_id, bcn_class_seq_id,
                           bcn_rssi, bcn_snr);
}

/**
 * Allocate a tx frame buffer if available.
 * \sa LPWAN_DEVICE_MAC_TX_FBUF_MAX_NUM
 * \sa mac_tx_fbuf_free()
 */
static struct tx_fbuf *mac_tx_fbuf_alloc(void)
{
    struct mem_pool_blk *blk = \
            mem_pool_alloc_blk(gl_uplink_frame_pool, sizeof(struct tx_fbuf));
    return (blk == NULL) ? NULL : ((struct tx_fbuf*) blk->blk);
}

/**
 * \sa mac_tx_fbuf_alloc()
 */
static void mac_tx_fbuf_free(const struct tx_fbuf *buffer)
{
    struct mem_pool_blk *blk = find_mem_pool_blk(buffer);
    mem_pool_free_blk(blk);
}

#ifdef __cplusplus
}
#endif

#endif /* TX_FRAMES_MGMT_C_ */
