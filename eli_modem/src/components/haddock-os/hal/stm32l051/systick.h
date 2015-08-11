/***************************************************************************************************
* @file name    hal_systick.h
* @data         2015/03/28
* @auther       chuanpengl
* @module       system tick
* @brief        system tick
***************************************************************************************************/
#ifndef __HAL_SYSTICK_H__
#define __HAL_SYSTICK_H__
/***************************************************************************************************
 * INCLUDES
 */


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
/* systick control */
#define SYSTICK_ENABLE()        SysTick->CTRL  = SysTick->CTRL | SysTick_CTRL_ENABLE_Msk; 
#define SYSTICK_DISABLE()       SysTick->CTRL  = SysTick->CTRL & (~SysTick_CTRL_ENABLE_Msk); 

/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */
typedef unsigned char systick_uint8;
typedef unsigned short systick_uint16;
typedef unsigned long systick_uint32;

typedef enum{systick_mode_run = 0, systick_mode_sleep = 1}systick_mode;

typedef void (*sysTick_UpdateTickCb)(systick_uint16 delta_ms);
/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * EXTERNAL FUNCTIONS DECLEAR
 */
/***************************************************************************************************
 * @fn      systick_Init()
 *
 * @brief   system tick init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_Init( sysTick_UpdateTickCb a_hTickUpdate);

/***************************************************************************************************
 * @fn      systick_SetWakeReload()
 *
 * @brief   reload tick set, in run mode = 1ms
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_SetWakeReload( void );

/***************************************************************************************************
 * @fn      systick_SetSleepReload()
 *
 * @brief   reload tick set, in sleep mode
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_SetSleepReload( systick_uint32 a_u32Cnt );

/***************************************************************************************************
 * @fn      systick_CalcTick()
 *
 * @brief   calc time escape in sleep
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_CalcTick( void );

/***************************************************************************************************
 * @fn      systick_Increase()
 *
 * @brief   system tick increase
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_Increase( void );


/***************************************************************************************************
 * @fn      systick_Get()
 *
 * @brief   get system tick
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
systick_uint32 systick_Get( void );

unsigned int HAL_GetTick(void);

#endif /* __HAL_SYSTICK_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
