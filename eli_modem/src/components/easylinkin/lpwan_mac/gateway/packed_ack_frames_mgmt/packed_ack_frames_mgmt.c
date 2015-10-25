/**
 * packed_ack_frames_mgmt.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */


#include "frame_defs/gw_beacon.h"
#include "gateway/mac_engine.h"

#include "kernel/hdk_memory.h"

#include "lib/ringbuffer.h"
#include "lib/assert.h"
#include "lib/mem_pool.h"

#include "lpwan_utils.h"
#include "packed_ack_frames_mgmt.h"

/*---------------------------------------------------------------------------*/
static struct ringbuffer *gl_packed_ack_list[GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM];
/**< Current list ID for @gl_packed_ack_list and @gl_downlink_frame_lists.
  init value: (negative)
  0-GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM
  \sa packed_ack_frames_has_ack() */
static os_int8 gl_cur_list_id;

struct downlink_frame_list {
    struct list_head flist;     /**< the frame buffer list \sa struct gw_downlink_fbuf */
    os_uint8 list_len;
};

static struct downlink_frame_list gl_downlink_frame_lists[GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM];

static os_uint8 gl_downlink_tx_list_len;
static struct list_head gl_downlink_tx_list;    /**< current tx downlink frames' list
                                                     \sa struct gw_downlink_fbuf */

static struct mem_pool_hdr *gl_downlink_frames_pool;

/*---------------------------------------------------------------------------*/
static void downlink_frame_clear_list(os_uint8 list_id);

static os_int8 calc_downlink_expected_list_id(os_int8 bcn_seq_id, os_int8 expected_bcn_seq_id);
static void downlink_frame_clear_list(os_uint8 list_id);

static os_boolean downlink_frames_is_within_tx_list(const struct gw_downlink_fbuf *fbuf);
/*---------------------------------------------------------------------------*/

/**
 * Malloc buffer for packed ACK management.
 */
void packed_ack_mgmt_alloc_buffer(void)
{
    for (os_uint8 i=0; i < GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM; i++) {
        gl_packed_ack_list[i] = \
                rbuf_new(GATEWAY_MAX_UPLINK_MSG_PER_BEACON_PERIOD,
                         sizeof(struct beacon_packed_ack));
        haddock_assert(gl_packed_ack_list[i] != NULL);
    }
}

/**
 * Init the current packed ACK list ID.
 */
void packed_ack_mgmt_init_list_id(os_int8 id)
{
    haddock_assert(id < 0);
    gl_cur_list_id = id;
}

/**
 * Init (reset) packed ACK buffer.
 */
void packed_ack_mgmt_reset_buffer(void)
{
    for (os_uint8 i=0; i < GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM; i++) {
        haddock_assert(gl_packed_ack_list[i]);

        gl_packed_ack_list[i]->hdr.len = 0;
        gl_packed_ack_list[i]->hdr.start = 0;
        gl_packed_ack_list[i]->hdr.end = 0;
    }
}

/**
 * Init the downlink frame list and frame buffer pool.
 */
void downlink_frames_mgmt_init(void)
{
    for (os_uint8 i=0; i < GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM; i++) {
        list_head_init(& gl_downlink_frame_lists[i].flist);
        gl_downlink_frame_lists[i].list_len = 0;
    }
    list_head_init(& gl_downlink_tx_list);

    gl_downlink_frames_pool = mem_pool_create(GATEWAY_DOWNLINK_FRAME_BUF_NUM,
                                              sizeof(struct gw_downlink_fbuf));
    haddock_assert(gl_downlink_frames_pool);
}

/**
 * Reset the downlink frames buffer.
 * \sa gateway_mac_engine_start()
 */
void downlink_frames_mgmt_reset(void)
{
    downlink_frame_clear_tx_list();
    // gl_downlink_frame_lists
    for (os_uint8 i=0; i < GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM; i++) {
        downlink_frame_clear_list(i);
    }
}

/**
 * \sa downlink_frame_free()
 * \sa GATEWAY_DOWNLINK_FRAME_BUF_NUM
 */
const struct gw_downlink_fbuf *downlink_frame_alloc(void)
{
    struct mem_pool_blk *blk = \
            mem_pool_alloc_blk(gl_downlink_frames_pool, sizeof(struct gw_downlink_fbuf));
    return (blk == NULL) ? NULL : ((void*) blk->blk);
}

/**
 * \sa downlink_frame_alloc()
 */
void downlink_frame_free(const struct gw_downlink_fbuf *fbuf)
{
    struct mem_pool_blk *blk = find_mem_pool_blk(fbuf);
    mem_pool_free_blk(blk);
}

/** Current downlink frame list's length.
 * \sa gl_cur_list_id */
os_uint8 downlink_frames_cur_list_len(void)
{
    if (gl_cur_list_id >= 0)
        return gl_downlink_frame_lists[gl_cur_list_id].list_len;
    else
        return 0;
}

os_uint8 downlink_frames_tx_list_len(void)
{
    return gl_downlink_tx_list_len;
}

/**
 * Update the packed ack information for each beacon period.
 */
void packed_ack_ticktock(void)
{
    os_int8 packed_ack_delay_num = (os_int8) gw_mac_info_get_packed_ack_delay_num();

    gl_cur_list_id += 1;
    if (gl_cur_list_id >= packed_ack_delay_num) {
        gl_cur_list_id = 0;
    }
}

/**
 * Calculate the expected packed ACK list ID from @packed_ack_delay_num.
 */
os_int8 packed_ack_expected_list_id(os_int8 packed_ack_delay_num)
{
    haddock_assert(packed_ack_delay_num > 0);
    return (gl_cur_list_id + packed_ack_delay_num) % packed_ack_delay_num;
}

os_uint8 packed_ack_cur_list_len(void)
{
    if (gl_cur_list_id >= 0)
        return rbuf_length(gl_packed_ack_list[gl_cur_list_id]);
    else
        return 0;
}

/**
 * Push the ACK header to the packed ACK list.
 */
os_int8 packed_ack_push(const struct beacon_packed_ack *ack)
{
    os_uint8 packed_ack_delay_num = gw_mac_info_get_packed_ack_delay_num();
    os_int8 expected_packed_ack_list_id = \
            packed_ack_expected_list_id((os_int8) packed_ack_delay_num);

    return rbuf_push_back(gl_packed_ack_list[expected_packed_ack_list_id],
                          ack, sizeof(struct beacon_packed_ack));
}

/**
 * \return NULL if no more ACK in the list, otherwise pop value to @ack and return @ack.
 */
struct beacon_packed_ack *packed_ack_pop(struct beacon_packed_ack *ack)
{
    haddock_assert(gl_cur_list_id >= 0);
    return rbuf_pop_front(gl_packed_ack_list[gl_cur_list_id],
                          ack, sizeof(struct beacon_packed_ack));
}

/**
 * Has packed ACK for current ack list?
 */
os_boolean has_packed_ack(void)
{
    return (packed_ack_cur_list_len() > 0);
}

/**
 * \return -1 if the beacon seq id is wrong. Non-negative value if ok.
 */
static os_int8 calc_downlink_expected_list_id(os_int8 bcn_seq_id, os_int8 expected_bcn_seq_id)
{
    os_int16 delta = calc_bcn_seq_delta(expected_bcn_seq_id, bcn_seq_id);
    if (0 < delta && delta <= GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM) {
        os_int16 packed_ack_delay_num = (os_int16) gw_mac_info_get_packed_ack_delay_num();
        return (os_int8) ((gl_cur_list_id + delta) % packed_ack_delay_num);
    } else {
        return -1;
    }
}

/**
 * @bcn_seq_id is current beacon sequence ID, @expected_bcn_seq_id is the expected
 * beacon sequence ID to tx downlink.
 *
 * \note @c_fbuf is allocated via downlink_frame_alloc()
 *
 * \return -1 if calculated beacon seq delta is invalid.
 *         0 if ok.
 */
os_int8 downlink_frame_put(const struct gw_downlink_fbuf *c_fbuf,
                           enum frame_type_gw type,
                           const struct lpwan_addr *dest,
                           const os_uint8 frame[], os_uint8 len,
                           os_int8 bcn_seq_id, os_int8 expected_bcn_seq_id)
{
    haddock_assert(c_fbuf);
    // only handle JOIN_CONFIRM or downlink message currently.
    haddock_assert((type == FTYPE_GW_JOIN_CONFIRMED && dest->type == ADDR_TYPE_MODEM_UUID)
                   ||
                   (type == FTYPE_GW_MSG && dest->type == ADDR_TYPE_SHORT_ADDRESS));
    haddock_assert(len <= LPWAN_RADIO_TX_BUFFER_MAX_LEN);

    os_int8 downlink_list_id = calc_downlink_expected_list_id(bcn_seq_id, expected_bcn_seq_id);
    if (downlink_list_id == -1) { // invalid
        return -1;
    }

    struct gw_downlink_fbuf *fbuf = (void *) c_fbuf;

    fbuf->type = type;
    fbuf->dest = *dest;
    fbuf->expected_downlink_list_id = downlink_list_id;
    fbuf->len = len;
    haddock_memcpy(fbuf->frame, frame, len);

    list_add_tail(& fbuf->hdr, & gl_downlink_frame_lists[downlink_list_id].flist);
    gl_downlink_frame_lists[downlink_list_id].list_len += 1;

    return 0;
}

/**
 * Check if downlink frame to @addr is available, and handle the downlink frmae if available.
 *
 * \note @addr is the address information contained in a single struct beacon_packed_ack.
 * \return found pending downlink frame?
 */
os_boolean downlink_frames_check(os_boolean is_join_ack, modem_short_t *addr)
{
    if (downlink_frames_cur_list_len() == 0)
        return OS_FALSE;

    haddock_assert(gl_cur_list_id >= 0);

    struct list_head *pos, *n;
    struct gw_downlink_fbuf *fbuf;

    os_boolean found = OS_FALSE;

    if (is_join_ack) {
        list_for_each_safe(pos, n, & gl_downlink_frame_lists[gl_cur_list_id].flist) {
            fbuf = list_entry(pos, struct gw_downlink_fbuf, hdr);

            if (fbuf->type != FTYPE_GW_JOIN_CONFIRMED)
                continue;
            haddock_assert(fbuf->dest.type == ADDR_TYPE_MODEM_UUID);

            short_modem_uuid_t suuid = short_modem_uuid(& fbuf->dest.addr.uuid);
            if (suuid == addr->suuid) {
                // matched!
                found = OS_TRUE;
                break;
            }
        }
    } else {
        list_for_each_safe(pos, n, & gl_downlink_frame_lists[gl_cur_list_id].flist) {
            fbuf = list_entry(pos, struct gw_downlink_fbuf, hdr);
            if (fbuf->type != FTYPE_GW_JOIN_CONFIRMED) {
                haddock_assert(fbuf->dest.type == ADDR_TYPE_SHORT_ADDRESS);
                if (fbuf->dest.addr.short_addr == addr->short_addr) {
                    // matched!
                    found = OS_TRUE;
                    break;
                }
            }
        }
    }

    if (found) {
        list_move_tail(& fbuf->hdr, & gl_downlink_tx_list);
        gl_downlink_tx_list_len += 1;
        gl_downlink_frame_lists[gl_cur_list_id].list_len -= 1;
    }

    return found;
}

static void downlink_frame_clear_list(os_uint8 list_id)
{
    haddock_assert(list_id < GATEWAY_DEFAULT_PACKED_ACK_DELAY_NUM);
    if (gl_downlink_frame_lists[list_id].list_len == 0)
        return;

    struct list_head *pos, *n;
    struct gw_downlink_fbuf *fbuf;
    list_for_each_safe(pos, n, & gl_downlink_frame_lists[list_id].flist) {
        fbuf = list_entry(pos, struct gw_downlink_fbuf, hdr);

        list_del_init(pos);
        downlink_frame_free(fbuf);
    }

    haddock_assert(list_empty(& gl_downlink_frame_lists[list_id].flist));
    gl_downlink_frame_lists[list_id].list_len = 0;
}

/**
 * Clear current downlink frames' list.
 * \sa gl_downlink_frame_lists
 * \sa gl_cur_list_id
 * \sa downlink_frames_mgmt_reset()
 */
void downlink_frame_clear_cur_list(void)
{
    if (downlink_frames_cur_list_len() == 0)
        return;

    haddock_assert(gl_cur_list_id >= 0);
    downlink_frame_clear_list(gl_cur_list_id);
}

/**
 * Clear the tx list.
 * \sa gl_downlink_tx_list
 * \sa gl_downlink_tx_list_len
 * \sa downlink_frames_mgmt_reset()
 */
void downlink_frame_clear_tx_list(void)
{
    struct list_head *pos, *n;
    struct gw_downlink_fbuf *fbuf;

    list_for_each_safe(pos, n, & gl_downlink_tx_list) {
        fbuf = list_entry(pos, struct gw_downlink_fbuf, hdr);

        list_del_init(pos);
        downlink_frame_free(fbuf);
    }
    gl_downlink_tx_list_len = 0;
}

/**
 * Get next frame buffer in @gl_downlink_tx_list.
 * \return NULL if no more downlink frame to tx.
 */
const struct gw_downlink_fbuf *downlink_frames_tx_list_get_next(void)
{
    const struct gw_downlink_fbuf *fbuf = NULL;

    if (! list_empty(& gl_downlink_tx_list)) {
        struct list_head *pos = gl_downlink_tx_list.next;
        fbuf = list_entry(pos, struct gw_downlink_fbuf, hdr);
    }

    return fbuf;
}

/**
 * Is @fbuf within @gl_downlink_tx_list?
 */
static os_boolean downlink_frames_is_within_tx_list(const struct gw_downlink_fbuf *fbuf)
{
    const struct list_head *fbuf_hdr = & fbuf->hdr;

    struct list_head *pos;
    list_for_each(pos, & gl_downlink_tx_list) {
        if (pos == fbuf_hdr)
            return OS_TRUE;
    }

    return OS_FALSE;
}

void downlink_frames_handle_tx_ok(const struct gw_downlink_fbuf *c_fbuf)
{
    haddock_assert(downlink_frames_is_within_tx_list(c_fbuf));

    struct gw_downlink_fbuf *fbuf = (void *) c_fbuf;
    list_del_init(& fbuf->hdr);
    downlink_frame_free(fbuf);

    gl_downlink_tx_list_len -= 1;
}

void downlink_frames_handle_tx_failed(const struct gw_downlink_fbuf *c_fbuf)
{
    haddock_assert(downlink_frames_is_within_tx_list(c_fbuf));

    struct gw_downlink_fbuf *fbuf = (void *) c_fbuf;
    list_del_init(& fbuf->hdr);
    downlink_frame_free(fbuf);

    gl_downlink_tx_list_len -= 1;
}
