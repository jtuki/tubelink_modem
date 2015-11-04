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

unsigned int sleepCnt = 0;

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
    
    GPIO_InitTypeDef  GPIO_InitStruct;

    __GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = (GPIO_PIN_6);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); 
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET);


    /* set systick in sleep mode */
    if( a_u16SleepTime > 10 ){
        systick_SetSleepReload(a_u16SleepTime - 5);
        SysTick->CTRL = 0;
        //SystemPower_Config__();

        /* sleep */
        //HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
        HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
        HAL_Init();
        clk_HsiPll32MHz();
        /* set wake tick after wake from stop mode */
        systick_setReloadAfterStopWake();
        sleepCnt ++;
    }
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_RESET);
}   /* slp_Request() */


#ifdef __cplusplus
}
#endif

#endif /* HAL_SLEEP_C_ */
