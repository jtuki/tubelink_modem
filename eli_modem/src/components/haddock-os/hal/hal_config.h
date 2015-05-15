/**
 * hal_config.h
 *
 * \date   
 * \author jtuki@foxmail.com
 */
#ifndef HADDOCK_HAL_CFG_H_
#define HADDOCK_HAL_CFG_H_

#include "hdk_user_config.h"

#if defined HDK_USER_CFG_MAIN_PC_TEST && HDK_USER_CFG_MAIN_PC_TEST == OS_TRUE
#include "pc_test/hal_config_pc_test.h"
#elif defined HDK_USER_CFG_MAIN_STM32F103 && HDK_USER_CFG_MAIN_STM32F103 == OS_TRUE
#include "stm32f103/hal_config_stm32f103.h"
#elif defined HDK_USER_CFG_MAIN_STM8L151 && HDK_USER_CFG_MAIN_STM8L151 == OS_TRUE
#include "stm8l151/hal_config_stm8l151.h"
#endif

#endif /* HADDOCK_HAL_CFG_H_ */
