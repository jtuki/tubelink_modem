/**
 * proc_2.c
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

static signal_bv_t proc_2_entry_test_set_event(os_pid_t pid, signal_bv_t signal);

os_pid_t gl_proc2_pid;
haddock_process("process_2");

#define customize_set_signal(dest_pid, signal) \
    if (t == NULL) { \
        t = os_timer_create((dest_pid), (signal), PROC_TEST_SET_SIGNAL_DELAY_MS); \
        haddock_assert(t); \
    } else { \
        os_timer_reconfig(t, (dest_pid), (signal), PROC_TEST_SET_SIGNAL_DELAY_MS); \
    } \
    os_timer_start(t);

void proc_2_init(void)
{
    struct process *proc2 = process_create(proc_2_entry_test_set_event, PROC2_PRIORITY);
    gl_proc2_pid = proc2->_pid;
}

static signal_bv_t proc_2_entry_test_set_event(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);
    static struct timer *t = NULL;
    
    if (signal & SIGNAL_SYS_MSG) {
        return signal ^ SIGNAL_SYS_MSG;
    }
    
    if (signal & PROC2_SIGNAL_1) {
        customize_set_signal(gl_proc3_pid, PROC3_SIGNAL_1);
        hdk_debug_print_set_signal(pid, gl_proc3_pid, PROC3_SIGNAL_1);
        return signal ^ PROC2_SIGNAL_1;
    }
    
    if (signal & PROC2_SIGNAL_2) {
        customize_set_signal(gl_proc3_pid, PROC3_SIGNAL_2);
        hdk_debug_print_set_signal(pid, gl_proc3_pid, PROC3_SIGNAL_2);
        return signal ^ PROC2_SIGNAL_2;
    }
    
    if (signal & PROC2_SIGNAL_3) {
        customize_set_signal(gl_proc3_pid, PROC3_SIGNAL_3);
        hdk_debug_print_set_signal(pid, gl_proc3_pid, PROC3_SIGNAL_3);
        return signal ^ PROC2_SIGNAL_3;
    }
    
    if (signal & PROC2_SIGNAL_4) {
        customize_set_signal(gl_proc3_pid, PROC3_SIGNAL_4);
        hdk_debug_print_set_signal(pid, gl_proc3_pid, PROC3_SIGNAL_4);
        return signal ^ PROC2_SIGNAL_4;
    }
    
    if (signal & PROC2_SIGNAL_5) {
        customize_set_signal(gl_proc3_pid, PROC2_SIGNAL_5);
        hdk_debug_print_set_signal(pid, gl_proc3_pid, PROC2_SIGNAL_5);
        return signal ^ PROC2_SIGNAL_5;
    }
    
    // unkown signal? discard.
    return 0;
}
