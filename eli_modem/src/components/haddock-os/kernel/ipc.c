/**
 * ipc.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "kernel_config.h"

#include "lib/assert.h"
#include "lib/mem_pool.h"
#include "lib/ringbuffer.h"

#include "hal/hal_mcu.h"
#include "sys_signal_defs.h"
#include "ipc.h"

/**
 * Return OS_TRUE if only one bit is set in @sig.
 */
static os_boolean is_signal(signal_t sig)
{
    for (os_size_t i=0; i < 32; i++) {
        if (sig == (((os_uint32)1)<<(i)))
            return OS_TRUE;
    }
    return OS_FALSE;
}

/**
 * \sa macro os_ipc_set_signal
 */
void __haddock_ipc_set_signal(struct process *proc, signal_t sig)
{
    haddock_assert(proc);
    haddock_assert(is_signal(sig));
    
    HDK_CRITICAL_STATEMENTS(proc->signal |= sig);
}

/**
 * Only the process itself can clear a triggered signal.
 * \sa macro os_ipc_clear_signal
 */
void __haddock_ipc_clear_signal(struct process *this, signal_t sig)
{
    haddock_assert(this);
    haddock_assert(is_signal(sig));

    HDK_CRITICAL_STATEMENTS(this->signal &= ~sig);
}

#if defined HDK_CFG_PROC_ENABLE_IPC_MSG && HDK_CFG_PROC_ENABLE_IPC_MSG == OS_TRUE

static struct mem_pool_hdr *ipc_msg_pool_list[HDK_CFG_IPC_MSG_CLASSES_NUM];

/**
 * If HDK_CFG_PROC_ENABLE_IPC_MSG is OS_TRUE, call this function to initialize
 * the ipc_msg poll buffers.
 */
void haddock_ipc_msg_module_init(void)
{
    const os_size_t *capacity_list = haddock_ipc_msg_classes_capacity;
    const os_size_t *blk_size_list = haddock_ipc_msg_classes_blk_size;
    os_size_t classes_num = sizeof(haddock_ipc_msg_classes_capacity);
    
#if (defined HADDOCK_DEBUG_OS_VERIFY_IPC_MSG_PARAMS) && HADDOCK_DEBUG_OS_VERIFY_IPC_MSG_PARAMS == OS_TRUE 
    // verify the ipc_msg related parameters in kernel_config.h.
    haddock_assert(classes_num == HDK_CFG_IPC_MSG_CLASSES_NUM);
    haddock_assert(sizeof(haddock_ipc_msg_classes_blk_size) == classes_num);
    
    os_size_t _total = 0;
    for (os_size_t i=0; i < classes_num; i++) {
        haddock_assert(capacity_list[i] > 0 && blk_size_list[i] > 0);
        if (i > 0) {
            // increase monotonically
            haddock_assert(blk_size_list[i] > blk_size_list[i-1]);
        }
        _total += capacity_list[i];
    }
    haddock_assert(_total == HDK_CFG_IPC_MSG_TOTAL_CAPACITY);
#endif
    
    for (os_size_t i=0; i < HDK_CFG_IPC_MSG_CLASSES_NUM; i++) {
        ipc_msg_pool_list[i] = mem_pool_create(capacity_list[i], blk_size_list[i]);
        haddock_assert(ipc_msg_pool_list[i] != NULL);
    }
}

static struct ipc_msg *__haddock_ipc_msg_alloc(os_size_t n);
static void __haddock_ipc_msg_free(struct ipc_msg *msg);

/**
 * \sa macro os_ipc_msg_create
 */
struct ipc_msg *__haddock_ipc_msg_create(struct process *src,
                                         os_uint8 msg[], os_uint16 len)
{
    struct ipc_msg *_ipc_msg = __haddock_ipc_msg_alloc(len);
    if (! _ipc_msg)
        return NULL;
    
    _ipc_msg->len = len;
    _ipc_msg->src_pid = src->_pid;
    haddock_memcpy(_ipc_msg->data, msg, len);
    
    return _ipc_msg;
}

/**
 * @return  0: ok
 * @return -1: dest_proc's msg_queue is full.
 */
int __haddock_ipc_send_msg(struct process *dest_proc, struct ipc_msg *msg)
{
    haddock_assert(msg && msg->src_pid != PROCESS_ID_RESERVED);
    if (rbuf_push_back(dest_proc->msg_queue, &msg, sizeof(void*)) == -1)
        return -1;
    
    msg->ref_cnt += 1;
    return 0;
}

/**
 * Return NULL if no more ipc_msg pending.
 * \note If the ipc_msg is sent to multiple destination process(es),
 *       the destination process(es) should not change the content of 
 *       the message (i.e. struct ipc_msg::data).
 */
struct ipc_msg *__haddock_ipc_receive_msg(struct process *dest_proc)
{
    haddock_assert(dest_proc && dest_proc->msg_queue);
    
    struct ipc_msg *msg = NULL;
    rbuf_pop_front(dest_proc->msg_queue, &msg, sizeof(void *));
    
    if (msg) {
        msg->ref_cnt -= 1;
        if (msg->ref_cnt == 0) {
            /**
             * Mark the block as free. The content remains the same.
             * \sa __haddock_ipc_msg_free()
             */
            __haddock_ipc_msg_free(msg);
        }
    }
    
    return msg;
}

static os_size_t find_suitable_mem_pool(os_size_t n,
                                     const os_size_t blk_size_list[])
{
    haddock_assert(n <= blk_size_list[HDK_CFG_IPC_MSG_CLASSES_NUM-1]);
    
    os_size_t i = 0;
    for (; i < HDK_CFG_IPC_MSG_CLASSES_NUM; i++) {
        if (n <= blk_size_list[i])
            break;
    }
    
    return i; 
}

/**
 * @param n The payload size of struct ipc_msg (i.e. struct ipc_msg::data).
 */
static struct ipc_msg *__haddock_ipc_msg_alloc(os_size_t n)
{
    const os_size_t *blk_size_list = haddock_ipc_msg_classes_blk_size;
    os_size_t _size = sizeof(struct ipc_msg) + n;
    
    haddock_assert(_size <= blk_size_list[HDK_CFG_IPC_MSG_CLASSES_NUM-1]);

    os_size_t min_idx = find_suitable_mem_pool(_size, blk_size_list);
    struct mem_pool_blk *blk = NULL;
    
    for (; min_idx < HDK_CFG_IPC_MSG_CLASSES_NUM && blk == NULL; ++min_idx) {
        blk = mem_pool_alloc_blk(ipc_msg_pool_list[min_idx], blk_size_list[min_idx]); 
    }

    if (blk) {
        haddock_memset(blk->blk, 0, blk->blk_size);
        return (struct ipc_msg *) blk->blk;
    } else {
        return NULL;
    }
}

static void __haddock_ipc_msg_free(struct ipc_msg *msg)
{
    haddock_assert(msg && msg->ref_cnt == 0);
    struct mem_pool_blk *mem_blk = find_mem_pool_blk(msg);
    
    /**
     * We actually mark it as free.
     * \sa __haddock_ipc_receive_msg()
     */
    mem_pool_free_blk(mem_blk);
}

#endif // HDK_CFG_PROC_ENABLE_IPC_MSG
