/***************************************************************************************************
* @file name    systick.c
* @data         2015/07/06
* @auther       chuanpengl
* @module       system tick
* @brief        system tick
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "systick.h"
#include "stm32l0xx_hal.h"
#include "hal_config_stm32l051.h"


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define SYSTICK_RUN_RELOAD_COUNTER      (249)
#define SYSTICK_SLEEP_RELOAD_COUNTER    (58)
#define SYSTICK_RUN_TIMER_CLK           (250000)    /* Hz */
#define SYSTICK_SLEEP_TIMER_CLK         (38000/64)  /* Hz */


#define SYSTICK_WAKE_CLK                (32000000)
#define SYSTICK_SLEEP_CLK               (32768)
#define SYSTICK_WAKE_PERIOD             (SYSTICK_WAKE_CLK/1000)
#define SYSTICK_SLEEP_PERIOD(_ms)       (SYSTICK_SLEEP_CLK*_ms/1000)
#define SYSTICK_WAKE_CNT_TO_US(_tick)   (_tick*1000/SYSTICK_WAKE_PERIOD)
#define SYSTICK_SLEEP_CNT_TO_US(_tick)  (_tick*1000*1000 >> 15 )


#define SYSTICK_GET_COUNT()     SysTick->VAL
/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    systick_uint32 u32Ms;
    systick_uint32 u32delta;    /* delta time */
}systickTime_t;

/* remember tick, after wakeup, calculate time and add to sys tick */
typedef struct
{
    systick_uint32 u32WakeTick;
    systick_uint32 u32SleepTick;
}systickRemTick_t;

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
volatile static systickTime_t gs_tSystickTimeLastUpdate, gs_tSystickTime;
static systick_mode gs_eSystickMode;
static systickRemTick_t gs_tTickRem;
static systick_uint32 gs_u32SleepTime;
/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */




 
/***************************************************************************************************
 *  EXTERNAL FUNCTIONS IMPLEMENTATION
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
void systick_Init( sysTick_UpdateTickCb a_hTickUpdate )
{
    gs_tSystickTime.u32delta = 0;
    gs_tSystickTime.u32Ms = 0;
    gs_tSystickTimeLastUpdate.u32delta = 0;
    gs_tSystickTimeLastUpdate.u32Ms = 0;
    
    gs_eSystickMode = systick_mode_run;

    /*Configure the SysTick to have interrupt in 1ms time basis*/
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /*Configure the SysTick IRQ priority */
    HAL_NVIC_SetPriority(SysTick_IRQn, INT_PRIORITY_SYSTICK ,0);
    
    SYSTICK_ENABLE();
    
}   /* systick_Init() */


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
void systick_SetWakeReload( void )
{
    gs_tTickRem.u32SleepTick = SYSTICK_GET_COUNT();
    SYSTICK_DISABLE();
    HAL_SYSTICK_Config(SYSTICK_WAKE_PERIOD );
}   /* systick_SetWakeReload() */

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
void systick_SetSleepReload( systick_uint32 a_u32Cnt )
{
    /*  */
    //assert_failed( a_u32Cnt < 100 );
    /* remember count */
    gs_u32SleepTime = a_u32Cnt;
    gs_tTickRem.u32WakeTick = SYSTICK_GET_COUNT();
    SYSTICK_DISABLE();
    HAL_SYSTICK_Config(SYSTICK_SLEEP_PERIOD(a_u32Cnt));
}

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
void systick_CalcTick( void )
{
    gs_tSystickTime.u32delta += SYSTICK_SLEEP_CNT_TO_US(gs_tTickRem.u32SleepTick);
    gs_tSystickTime.u32delta += SYSTICK_WAKE_CNT_TO_US(gs_tTickRem.u32WakeTick);
    gs_tSystickTime.u32Ms += (gs_tSystickTime.u32delta / 100);
    gs_tSystickTime.u32delta = gs_tSystickTime.u32delta % 100;
}


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
void systick_Increase( void )
{

    if(gs_eSystickMode == systick_mode_run)
    {
        gs_tSystickTime.u32Ms ++;
    }
    else
    {
        gs_tSystickTime.u32Ms += gs_u32SleepTime;
    }

}   /* systick_Increase() */

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
systick_uint32 systick_Get( void )
{
    return gs_tSystickTime.u32Ms;
}   /* systick_Get() */


unsigned int HAL_GetTick(void)
{
    return gs_tSystickTime.u32Ms;
}
/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
 


/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150706
*   context: create file
*
***************************************************************************************************/
