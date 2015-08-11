/***************************************************************************************************
* @file name    sleep.c
* @data         2015/07/06
* @auther       chuanpengl
* @module       sleep module
* @brief        sleep module
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "sleep.h"

#include "systick.h"
#include "stm32l0xx_hal.h"

#include <assert.h>

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define slpAssert           assert
#define WAKEUP_TICK_REEQ            (1187.5)  /* 1187.5Hz */

/***************************************************************************************************
 * TYPEDEFS
 */
 
 
/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
 

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */


/***************************************************************************************************
 * EXTERNAL VARIABLES
 */
extern void clk_SleepConfig(void);
extern void clk_HsiPll32MHz(void);

 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      slp_Request()
 *
 * @brief   request to sleep for some time
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_Request( slpUint16 a_u16SleepTime, slpUint8 *a_pu8SlpAssert )
{
    slpUint16 u16ReloadCnt = 0;
    
    /* set systick in sleep mode */
    systick_SetSleepReload(a_u16SleepTime);
    /* set clock  */
    clk_SleepConfig();
    /* enable sys timer */
    SYSTICK_ENABLE();

    /* sleep */
    //HAL_PWR_EnterSTANDBYMode();
    //HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    clk_HsiPll32MHz();
    systick_SetWakeReload();
    SYSTICK_ENABLE();
    systick_CalcTick();
    
}   /* slp_Request() */

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
 

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150706
*   context: here write modified history
*
***************************************************************************************************/
