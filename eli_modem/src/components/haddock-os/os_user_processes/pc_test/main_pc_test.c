/**
 * main_pc_test.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "hdk_user_config.h"
#include "processes/pc_test_processes.h"

void os_processes_init_pc_test(void) {
    proc_1_init();
    proc_2_init();
    proc_3_init();
    proc_4_init();
    proc_5_init();
    // proc_timetick_init();
}
