/**
 * kernel_main.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "hal/hal_config.h"
#include "hal/hal_mcu.h"

#include "kernel/kernel_config.h"
#include "kernel/process.h"
#include "kernel/timer.h"
#include "kernel/ipc.h"
#include "kernel/hdk_memory.h"
#include "kernel/scheduler.h"
#include "kernel/power_manager.h"
#include "os_user_processes/os_user_processes.h"

#include "lib/assert.h"

static os_uint32 __kernel_main_loop_counter = 0;

void main ()
{
    // hardware related initialization.
    haddock_hal_init();
    
    // kernel resource initialization.
    haddock_process_module_init();
    haddock_timer_module_init();

#if (defined HDK_CFG_PROC_ENABLE_IPC_MSG) && HDK_CFG_PROC_ENABLE_IPC_MSG == OS_TRUE
    haddock_ipc_msg_module_init();
#endif

#if defined HDK_CFG_POWER_SAVING_ENABLE && HDK_CFG_POWER_SAVING_ENABLE == OS_TRUE
    haddock_power_manager_module_init();
#endif
    
    os_user_processes_init();
    
    static struct process *proc = NULL;
    /**
     * \remark We use @_signal to backup the process's signal_bv_t before the
     *         execution of @proc->entry to avoid the race conditions between 
     *         interrupts (who may want to set_signal) and @proc->entry (who
     *         may want to clear_signal).
     */
    signal_bv_t _signal = 0;
    interrupt_state_t _state;
    
    while (OS_TRUE) {
        __kernel_main_loop_counter += 1;
        
        haddock_timer_update_routine();
        proc = schedule_next();
        if (proc) {
#if defined HDK_CFG_POWER_SAVING_ENABLE && HDK_CFG_POWER_SAVING_ENABLE == OS_TRUE
            haddock_power_status = POWER_STATUS_RUNNING;
#endif
            
#if defined HDK_CFG_POWER_SAVING_ENABLE && HDK_CFG_POWER_SAVING_ENABLE == OS_TRUE
            if (proc->is_wakeup == OS_FALSE) {
                haddock_disable_power_conserve(proc);
            }
#endif

            HDK_ENTER_CRITICAL_SECTION(_state);
            _signal = proc->signal;
            proc->signal = 0;
            HDK_EXIT_CRITICAL_SECTION(_state);

            haddock_assert(proc->entry);
            cur_process_id = proc->_pid;
            
            _signal = proc->entry(proc->_pid, _signal);
            
            HDK_ENTER_CRITICAL_SECTION(_state);
            proc->signal |= _signal;
            HDK_EXIT_CRITICAL_SECTION(_state);
        }
#if defined HDK_CFG_POWER_SAVING_ENABLE && HDK_CFG_POWER_SAVING_ENABLE == OS_TRUE
        else {
            /**
             * Currently, there is no more process is scheduled to be running.
             * So we try to go to power conservation mode.  
             */
            haddock_power_conserve_routine();
        }
#endif
    }
}
