/**
 * hal_stm32f103.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef HAL_STM32F103_H_
#define HAL_STM32F103_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "types.h"

typedef unsigned int interrupt_state_t;

/**
 * todo #mark#
 */
#define HDK_ENTER_CRITICAL_SECTION(s) {(void)s;}
#define HDK_EXIT_CRITICAL_SECTION(s) {(void)s;}

#define HDK_CRITICAL_STATEMENTS(statements) \
    do {interrupt_state_t _state; HDK_ENTER_CRITICAL_SECTION(_state); \
        statements; HDK_EXIT_CRITICAL_SECTION(_state);} while (0)

#define haddock_hal_init() \
    __haddock_hal_stm32f103_init()

#define haddock_hal_sleep(delta_ms) \
    __haddock_hal_stm32f103_sleep(delta_ms)

extern void __haddock_hal_stm32f103_init(void);
extern void __haddock_hal_stm32f103_sleep(uint16 delta_ms);

#ifdef __cplusplus
}
#endif

#endif /* HAL_STM32F103_H_ */
