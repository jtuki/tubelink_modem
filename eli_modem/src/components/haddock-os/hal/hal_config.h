/**
 * hal_config.h
 *
 * \date   
 * \author jtuki@foxmail.com
 */
#ifndef HADDOCK_HAL_CFG_H_
#define HADDOCK_HAL_CFG_H_

#include "hdk_user_config.h"

#if defined HDK_USER_CFG_HAL_PC && HDK_USER_CFG_HAL_PC == OS_TRUE
#include "pc_test/hal_config_pc_test.h"
#elif defined HDK_USER_CFG_HAL_ELI_MODEM_STM32F103 && HDK_USER_CFG_HAL_ELI_MODEM_STM32F103 == OS_TRUE
#include "stm32f103/hal_config_stm32f103.h"
#elif defined HDK_USER_CFG_HAL_ELI_MODEM_STM8L151 && HDK_USER_CFG_HAL_ELI_MODEM_STM8L151 == OS_TRUE
#include "stm8l151/hal_config_stm8l151.h"
#elif defined HDK_USER_CFG_HAL_ELI_MODEM_STM32L051 && HDK_USER_CFG_HAL_ELI_MODEM_STM32L051 == OS_TRUE
#include "stm32l051/hal_stm32l051.h"
#endif

#endif /* HADDOCK_HAL_CFG_H_ */
