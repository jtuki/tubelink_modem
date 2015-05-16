/**
 * hal_mcu.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef HAL_MCU_H_
#define HAL_MCU_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "hdk_user_config.h"

/**
 * Each HAL (hardware abstraction layer) implementation should implement:
 * 1. HDK_ENTER_CRITICAL_SECTION(s)
 * 2. HDK_ENTER_CRITICAL_SECTION(s)
 * 3. HDK_CRITICAL_STATEMENTS(statements)
 * 4. void haddock_hal_init(void)
 * 5. void haddock_hal_sleep(uint16 delta_ms)
 * 
 * \sa stm32f103/hal_stm32f103.h
 */

#if defined HDK_USER_CFG_HAL_PC && HDK_USER_CFG_HAL_PC == OS_TRUE
#include "pc_test/hal_pc_test.h"
#elif defined HDK_USER_CFG_HAL_ELI_MODEM_STM32F103 && HDK_USER_CFG_HAL_ELI_MODEM_STM32F103 == OS_TRUE
#include "stm32f103/hal_stm32f103.h"
#elif defined HDK_USER_CFG_HAL_ELI_MODEM_STM8L151 && HDK_USER_CFG_HAL_ELI_MODEM_STM8L151 == OS_TRUE
#include "stm8l151/hal_stm8l151.h"
#endif


#ifdef __cplusplus
}
#endif

#endif /* HAL_MCU_H_ */
