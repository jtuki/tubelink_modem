/**
 * hal_config_stm32f103.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef HAL_CONFIG_STM32L501_H_
#define HAL_CONFIG_STM32L501_H_

#include "stm32l0xx_hal.h"
#include "haddock_types.h"
#include "systick.h"

#ifdef __cplusplus
extern "C"
{
#endif

// configuration of the HAL for stm32L051

/**
 * Priority configuration. Lower value, higher priority.
 * \sa HAL_NVIC_SetPriority()
 */
#define INT_PRIORITY_SYSTICK    0   // use sysTick to update system clock
#define INT_PRIORITY_UART       3   // use UART for external communication
#define INT_PRIORITY_RADIO      1   // use SPI for radio interaction

#ifdef __cplusplus
}
#endif

#endif /* HAL_CONFIG_STM32F103_H_ */
