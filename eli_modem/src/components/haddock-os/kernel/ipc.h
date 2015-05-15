/**
 * ipc.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef IPC_MSG_H_
#define IPC_MSG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "haddock_types.h"
#include "kernel/kernel_config.h"
#include "kernel/process.h"
#include "kernel/power_manager.h"

/**
 * Only one bit is set in this signal_t.
 * \sa struct process::signal
 * \sa signal_bv_t
 */
typedef os_uint32 signal_t;

/**
 * _Should_ only be called in interrupt service routine.
 */
#define os_interrupt_ipc_set_signal(pid, sig) \
    haddock_power_status = POWER_STATUS_ISR_SET_SIGNAL; \
    __haddock_ipc_set_signal(get_struct_process(pid), sig)

#define os_ipc_set_signal(pid, sig) \
    __haddock_ipc_set_signal(get_struct_process(pid), sig)

#define os_ipc_clear_signal(sig) \
    __haddock_ipc_clear_signal(this, signal_t sig)

void __haddock_ipc_set_signal(struct process *proc, signal_t sig);
void __haddock_ipc_clear_signal(struct process *this, signal_t sig);

#if defined HDK_CFG_PROC_ENABLE_IPC_MSG && HDK_CFG_PROC_ENABLE_IPC_MSG == OS_TRUE

/**
 * Inter-process communication message.
 * \note The ipc message can be sent from itself to itself.
 * 
 * Only the source/sender which generates the message is known.
 * All sink/receiver share a message's memory block, and use @ref_cnt to decide
 * whether or not free the message after receive and handle it.
 */
struct ipc_msg {
    os_uint16 ref_cnt;    /**< When @ref_cnt decrease to 0, free the ipc_msg. */
    os_uint16 len;        /**< length of @data */
    os_pid_t src_pid; /**< The source where the ipc_msg is generated. */
    os_uint8 data[0];
};

void haddock_ipc_msg_module_init(void);

#define os_ipc_msg_create(msg, len) \
    __haddock_ipc_msg_create(this, msg, len)

#define os_ipc_send_msg(pid, msg) \
    __haddock_ipc_send_msg(get_struct_process(pid), msg)

#define os_ipc_receive_msg() \
    __haddock_ipc_receive_msg(this)

struct ipc_msg *__haddock_ipc_msg_create(struct process *src,
                                         os_uint8 msg[], os_uint16 len);
int __haddock_ipc_send_msg(struct process *dest_proc, struct ipc_msg *msg);
struct ipc_msg *__haddock_ipc_receive_msg(struct process *dest_proc);

#endif // HDK_CFG_PROC_ENABLE_IPC_MSG

#ifdef __cplusplus
}
#endif

#endif /* IPC_MSG_H_ */
