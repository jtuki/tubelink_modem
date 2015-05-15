/**
 * hdk_user_config.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef HDK_USER_CONFIG_H_
#define HDK_USER_CONFIG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "haddock_types.h"

/**
 * _ONLY_ one is available for main(). Comment all the others.
 */
#define HDK_USER_CFG_LPWAN_MAC          OS_TRUE
// #define HDK_USER_CFG_MAIN_PC_TEST       OS_TRUE
// #define HDK_USER_CFG_MAIN_EXAMPLE_USER  OS_TRUE
#define HDK_USER_CFG_ELI_ENDNODE        OS_TRUE

    
    
#define HDK_USER_CFG_MAIN_STM8L151     OS_TRUE
#ifdef __cplusplus
}
#endif

#endif /* HDK_USER_CONFIG_H_ */
