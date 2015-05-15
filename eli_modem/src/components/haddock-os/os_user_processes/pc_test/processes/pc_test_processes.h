/**
 * pc_test_processes.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef PC_TEST_PROCESSES_H_
#define PC_TEST_PROCESSES_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel/process.h"
#include "lib/hdk_utilities.h"
#include "haddock_types.h"

#define hdk_debug_print_set_signal(src_pid, dest_pid, signal_name) \
    haddock_debug_print_set_signal(src_pid, dest_pid, #signal_name)

void haddock_debug_print_set_signal(os_pid_t src_pid, os_pid_t dest_pid,
                                    const char *signal_name);
void hdk_debug_print(const char *string, ...);

#define PROC1_PRIORITY          4
#define PROC2_PRIORITY          3
#define PROC3_PRIORITY          6
#define PROC4_PRIORITY          0
#define PROC5_PRIORITY          1

/**
 * If proc1's signal N is set, it will set proc2's signal N.
 * If proc2's signal N is set, it will set proc3's signal N.
 * ... etc ...
 * If proc5's signal N (N<5) is set, it will set proc1's signal (N+1).
 * ...
 * If proc5's signal 5 is set, the processes's job are done.
 */

#define PROC1_SIGNAL_1          BV(0)
#define PROC1_SIGNAL_2          BV(3)
#define PROC1_SIGNAL_3          BV(6)
#define PROC1_SIGNAL_4          BV(9)
#define PROC1_SIGNAL_5          BV(12)

#define PROC2_SIGNAL_1          BV(0)
#define PROC2_SIGNAL_2          BV(3)
#define PROC2_SIGNAL_3          BV(6)
#define PROC2_SIGNAL_4          BV(9)
#define PROC2_SIGNAL_5          BV(12)

#define PROC3_SIGNAL_1          BV(0)
#define PROC3_SIGNAL_2          BV(3)
#define PROC3_SIGNAL_3          BV(6)
#define PROC3_SIGNAL_4          BV(9)
#define PROC3_SIGNAL_5          BV(12)

#define PROC4_SIGNAL_1          BV(0)
#define PROC4_SIGNAL_2          BV(3)
#define PROC4_SIGNAL_3          BV(6)
#define PROC4_SIGNAL_4          BV(9)
#define PROC4_SIGNAL_5          BV(12)

#define PROC5_SIGNAL_1          BV(0)
#define PROC5_SIGNAL_2          BV(3)
#define PROC5_SIGNAL_3          BV(6)
#define PROC5_SIGNAL_4          BV(9)
#define PROC5_SIGNAL_5          BV(12)

#define PROC_TIMETICK_SIGNAL_INCREMENT  BV(20)

#define PROC_TEST_SET_SIGNAL_DELAY_MS 30

extern os_pid_t gl_proc1_pid;
extern os_pid_t gl_proc2_pid;
extern os_pid_t gl_proc3_pid;
extern os_pid_t gl_proc4_pid;
extern os_pid_t gl_proc5_pid;

void proc_1_init(void);
void proc_2_init(void);
void proc_3_init(void);
void proc_4_init(void);
void proc_5_init(void);
void proc_timetick_init(void);

#ifdef __cplusplus
}
#endif

#endif /* PC_TEST_PROCESSES_H_ */
