/**
 * sys_signal_defs.h
 *
 * \date   
 * \author jtuki@foxmail.com
 */
#ifndef HADDOCK_SYS_SIGNAL_DEFS_H_
#define HADDOCK_SYS_SIGNAL_DEFS_H_

/******************************************************************************
 * All system-related message and signal are defined here.
 * 
 * _SPECIAL CARE_
 *      ANY user-defined signal or message should not conflict with the system
 *      signal/message defined here, or unexpected behavior might happen.
 */

/**
 * The highest bit of the @signal bit-vector within struct process is reserved
 * for the system message, i.e. we use this bit to notify the arriving of ipc
 * messages (struct ipc_msg). 
 */
#define SIGNAL_SYS_MSG                  (((os_uint32)1) << 31)

/**
 * When a process is blocked to wait for some signal/message to leave "wait" 
 * mode (i.e. transit to "ready" mode for scheuler to schedule), we use this bit 
 * to indicate that the block period is timeout (leave "wait" mode not because 
 * of the arriving of a signal/message but due to timeout).
 */
#define SIGNAL_SYS_BLOCK_TIMEOUT        (((os_uint32)1) << 30)

#endif /* HADDOCK_SYS_SIGNAL_DEFS_H_ */
