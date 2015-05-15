/**
 * pc_test_processes.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "kernel/timer.h"
#include "kernel/process.h"
#include "pc_test_processes.h"
#include <stdarg.h>
#include <stdio.h>

void haddock_debug_print_set_signal(os_pid_t src_pid, os_pid_t dest_pid,
                                    const char *signal_name)
{
    struct time t;
    haddock_get_time_tick_now(&t);
}

/* refer to:
    http://www.velocityreviews.com/forums/t437343-printf-wrapper.html
    http://www.cplusplus.com/reference/cstdio/vprintf/
 */
void hdk_debug_print(const char *string, ...)
{
    struct time t;
    haddock_get_time_tick_now(&t);
    
    //va_list args;
    //va_start(args, string);
    
    //fprintf(stdout, "(time: %3d:%4d) DEBUG_PRINT - ", t.s, t.ms);
    //vfprintf(stdout, string, args);
    //fprintf(stdout, "\n");
    
    //va_end(args);
}
