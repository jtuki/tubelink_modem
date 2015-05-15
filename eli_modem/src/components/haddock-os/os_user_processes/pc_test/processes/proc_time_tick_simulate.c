/**
 * proc_time_tick_simulate.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "kernel/process.h"
#include "kernel/timer.h"
#include "kernel/ipc.h"
#include "kernel/sys_signal_defs.h"
#include "lib/assert.h"
#include "pc_test_processes.h"

static signal_bv_t proc_timetick_simulate(os_pid_t pid, signal_bv_t signal);

os_pid_t gl_proc_timetick_pid;
haddock_process("proc_timetick");

void proc_timetick_init(void)
{
    struct process *proc_timetick = \
            process_create(proc_timetick_simulate, HDK_CFG_PROC_PRIORITY_NUM-1);
    gl_proc_timetick_pid = proc_timetick->_pid;
    
    // start time tick
    os_ipc_set_signal(this->_pid, PROC_TIMETICK_SIGNAL_INCREMENT);
}

static signal_bv_t proc_timetick_simulate(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);
    
    if (signal & SIGNAL_SYS_MSG) {
        return signal ^ SIGNAL_SYS_MSG;
    }
    
    if (signal & PROC_TIMETICK_SIGNAL_INCREMENT) {
        __haddock_increment_time_tick_now(1);
        os_ipc_set_signal(this->_pid, PROC_TIMETICK_SIGNAL_INCREMENT);
        return signal ^ PROC_TIMETICK_SIGNAL_INCREMENT;
    }
    
    // unkown signal? discard.
    return 0;
}
