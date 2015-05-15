/***************************************************************************************************
* @file name	hal_stm8l151.h
* @data   		2015/04/23
* @auther   	chuanpengl
* @module   	cpu operation for stm8l151
* @brief 		will provide critical section methods
***************************************************************************************************/
#ifndef __HAL_STM8L151_H__
#define __HAL_STM8L151_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "hal_config_stm8l151.h"


#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define HDK_ENTER_CRITICAL_SECTION(s) {s=s;__disable_interrupt();}
#define HDK_EXIT_CRITICAL_SECTION(s) {__enable_interrupt();s=s;}

#define HDK_CRITICAL_STATEMENTS(statements) \
    do {interrupt_state_t _state=0; HDK_ENTER_CRITICAL_SECTION(_state); \
        statements; HDK_EXIT_CRITICAL_SECTION(_state);} while (0)

/***************************************************************************************************
 * TYPEDEFS
 */
typedef unsigned int interrupt_state_t;

/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */
/***************************************************************************************************
 * @fn      haddock_hal_init()
 *
 * @brief   hal init for haddock
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void haddock_hal_init(void);

/***************************************************************************************************
 * @fn      haddock_hal_sleep()
 *
 * @brief   sleep, mcu will stop running in this function until wakeup by interrupt
 *
 * @author	chuanpengl
 *
 * @param   delta_ms  - sleep time, unit is ms
 *
 * @return  none
 */
void haddock_hal_sleep(os_uint16 delta_ms);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_STM8L151_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/