/**
 * kernel_config.h
 *
 * \date   
 * \author jtuki@foxmail.com
 */
#ifndef HADDOCK_KERNEL_CFG_H_
#define HADDOCK_KERNEL_CFG_H_

#include "haddock_types.h"
#include "hdk_user_config.h"

/******************************************************************************
 * Boolean value configuration.
 */

#define HDK_CFG_DEBUG                   OS_TRUE
#define HDK_CFG_PROC_WITH_NAME          OS_FALSE
#define HDK_CFG_PROC_ENABLE_IPC_MSG     OS_FALSE
#define HDK_CFG_DEFAULT_SCHEDULER       OS_TRUE /* refer to kernel/scheduler.c */

/******************************************************************************
 * Numerical value configuration.
 */

/**< Maximum process number. (max number is 32) 
 * \sa haddock_allow_power_conserve_bv_t */
#define HDK_CFG_PROC_MAX_NUM            2

/**< Maximum message queue size for each process. */
#define HDK_CFG_PROC_MAX_MSG_QUEUE_SIZE 5

/**< Higher the value is, nicer the process is (with low priority). 
     Priority is within range [0,7). */
#define HDK_CFG_PROC_PRIORITY_NUM       2

/**
 * The struct ipc_msg pool size.
 * i.e. sum(haddock_ipc_msg_classes_capacity)
 * \sa haddock_ipc_msg_classes_capacity
 * 
 * For each ipc_msg, the @data buffer size varies.
 * \sa haddock_ipc_msg_classes_blk_size
 */
#define HDK_CFG_IPC_MSG_TOTAL_CAPACITY  9
#define HDK_CFG_IPC_MSG_CLASSES_NUM     4

extern const os_size_t haddock_ipc_msg_classes_capacity[HDK_CFG_IPC_MSG_CLASSES_NUM];
extern const os_size_t haddock_ipc_msg_classes_blk_size[HDK_CFG_IPC_MSG_CLASSES_NUM];

/******************************************************************************
 * Memory allocation related configuration.
 */

/** memory reserevd for allocation, in bytes. 
 * \sa haddock_malloc() */
#define HDK_CFG_MEMORY_FOR_MALLOC       800

/******************************************************************************
 * Timer related configuration.
 */

#define HDK_CFG_TIMER_MAX_NUM           15

/** 15ms. \sa haddock_timer_update_routine() */
#define HDK_CFG_TIMER_SCHEDULE_THRESHOLD 15
/** 1 seconds \sa haddock_timer_sync() */
#define HDK_CFG_TIMER_SYNC_THRESHOLD    1

/** Does the system is synchronized with some external precise clock? */
#define HDK_CFG_TIMER_ENABLE_SYNC       OS_FALSE

/******************************************************************************
 * Power saving related configuration.
 */

#define HDK_CFG_POWER_SAVING_ENABLE     OS_TRUE

/** sleep if the next timer's timeout value exeeds @threshold ms. 
 * \sa haddock_power_conserve_routine() */
#define HDK_CFG_POWER_SAVING_NEXT_TIMEOUT_THRESHOLD     12

/**
 * \sa haddock_hal_sleep(uint16 delta_ms)
 */
#define HDK_CFG_POWER_SAVING_SLEEP_MAX_MS       65535 // ((1<<16)-1)

#endif /* HADDOCK_KERNEL_CFG_H_ */
