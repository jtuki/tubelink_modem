/**
 * hal_sleep.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef HAL_SLEEP_C_
#define HAL_SLEEP_C_

#ifdef __cplusplus
extern "C"
{
#endif

#include "hal_stm32l051.h"
#include "systick.h"

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
static void slp_Request( os_uint16 a_u16SleepTime );

void __haddock_hal_stm32l051_sleep(os_uint16 delta_ms)
{
    slp_Request(delta_ms);
}


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
void slp_Request( os_uint16 a_u16SleepTime )
{
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
    __enable_irq();
    clk_HsiPll32MHz();
    systick_SetWakeReload();
    SYSTICK_ENABLE();
    systick_CalcTick();
    
}   /* slp_Request() */


#ifdef __cplusplus
}
#endif

#endif /* HAL_SLEEP_C_ */
