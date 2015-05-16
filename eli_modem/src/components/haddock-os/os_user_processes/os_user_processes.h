/**
 * os_user_processes.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef OS_USER_PROCESSES_H_
#define OS_USER_PROCESSES_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "hdk_user_config.h"
#include "haddock_types.h"
#include "easylinkin_modem/easylinkin_modem_process.h"


    
#if defined HDK_USER_CFG_MAIN_PC_TEST && HDK_USER_CFG_MAIN_PC_TEST == OS_TRUE
#define os_user_processes_init() \
    os_processes_init_pc_test()

#elif (defined HDK_USER_CFG_MAIN_EXAMPLE_USER) && (HDK_USER_CFG_MAIN_EXAMPLE_USER == OS_TRUE)
#define os_user_processes_init() \
    os_processes_init_example_user()

#elif (defined HDK_USER_CFG_MAIN_ELI_MODEM) && (HDK_USER_CFG_MAIN_ELI_MODEM == OS_TRUE)
#define os_user_processes_init() \
        os_processes_init_easylinkin_modem()
#endif


#ifdef __cplusplus
}
#endif

#endif /* OS_USER_PROCESSES_H_ */
