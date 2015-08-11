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
 * Specify the main() function.
 *
 * \remark
 *      _ONLY_ one is available for main(). Comment all the others.
 *      prefix: HDK_USER_CFG_MAIN_xxx
 *
 * \sa HDK_USER_CFG_HAL_xxx
 */
// #define HDK_USER_CFG_MAIN_PC_TEST       OS_TRUE
// #define HDK_USER_CFG_MAIN_EXAMPLE_USER  OS_TRUE
#define HDK_USER_CFG_MAIN_ELI_MODEM        OS_TRUE

/**
 * Specify the HAL (hardware platform).
 *
 * \remark
 *      Corresponding to HDK_USER_CFG_MAIN_xxx.
 */
// #define HDK_USER_CFG_HAL_PC                     OS_TRUE
// #define HDK_USER_CFG_HAL_ELI_MODEM_STM32F103    OS_TRUE
//#define HDK_USER_CFG_HAL_ELI_MODEM_STM8L151     OS_TRUE
#define HDK_USER_CFG_HAL_ELI_MODEM_STM32L051     OS_TRUE


#ifdef __cplusplus
}
#endif

#endif /* HDK_USER_CONFIG_H_ */
