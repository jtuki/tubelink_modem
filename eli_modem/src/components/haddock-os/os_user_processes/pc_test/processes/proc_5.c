/**
 * proc_5.c
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

#include "stdlib.h"

static signal_bv_t proc_5_entry_test_set_event(os_pid_t pid, signal_bv_t signal);

os_pid_t gl_proc5_pid;
haddock_process("process_5");

#define customize_set_signal(dest_pid, signal) \
    if (t == NULL) { \
        t = os_timer_create((dest_pid), (signal), PROC_TEST_SET_SIGNAL_DELAY_MS); \
        haddock_assert(t); \
    } else { \
        os_timer_reconfig(t, (dest_pid), (signal), PROC_TEST_SET_SIGNAL_DELAY_MS); \
    } \
    os_timer_start(t);

void proc_5_init(void)
{
    struct process *proc5 = process_create(proc_5_entry_test_set_event, PROC5_PRIORITY);
    gl_proc5_pid = proc5->_pid;
}

static signal_bv_t proc_5_entry_test_set_event(os_pid_t pid, signal_bv_t signal)
{
    haddock_assert(pid == this->_pid);
    static struct timer *t = NULL;
    
    if (signal & SIGNAL_SYS_MSG) {
        return signal ^ SIGNAL_SYS_MSG;
    }
    
    if (signal & PROC5_SIGNAL_1) {
        customize_set_signal(gl_proc1_pid, PROC1_SIGNAL_2);
        hdk_debug_print_set_signal(pid, gl_proc1_pid, PROC1_SIGNAL_2);
        return signal ^ PROC5_SIGNAL_1;
    }
    
    if (signal & PROC5_SIGNAL_2) {
        customize_set_signal(gl_proc1_pid, PROC1_SIGNAL_3);
        hdk_debug_print_set_signal(pid, gl_proc1_pid, PROC5_SIGNAL_2);
        return signal ^ PROC5_SIGNAL_2;
    }
    
    if (signal & PROC5_SIGNAL_3) {
        customize_set_signal(gl_proc1_pid, PROC1_SIGNAL_4);
        hdk_debug_print_set_signal(pid, gl_proc1_pid, PROC1_SIGNAL_4);
        return signal ^ PROC5_SIGNAL_3;
    }
    
    if (signal & PROC5_SIGNAL_4) {
        customize_set_signal(gl_proc1_pid, PROC1_SIGNAL_5);
        hdk_debug_print_set_signal(pid, gl_proc1_pid, PROC1_SIGNAL_5);
        return signal ^ PROC5_SIGNAL_4;
    }
    
    if (signal & PROC5_SIGNAL_5) {
        hdk_debug_print("all processes are done.");
        exit(0);
        return signal ^ PROC5_SIGNAL_5;
    }
    
    // unkown signal? discard.
    return 0;
}
