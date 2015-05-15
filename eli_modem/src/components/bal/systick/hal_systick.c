/***************************************************************************************************
* @file name	hal_systick.c
* @data   		2015/03/28
* @auther   	chuanpengl
* @module   	system tick
* @brief 		system tick
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "hal_systick.h"

#if _SW_SYSTICK
#include "stm8l15x.h"

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

/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    systick_uint32 u32Ms;
    systick_uint16 u16delta;    /* delta time */
}systickTime_t;

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
static systickTime_t gs_tSystickTimeLastUpdate, gs_tSystickTime;
static systick_mode gs_eSystickMode;
static sysTick_UpdateTickCb gs_hTickUpdate;

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
    gs_tSystickTime.u16delta = 0;
    gs_tSystickTime.u32Ms = 0;
    gs_tSystickTimeLastUpdate.u16delta = 0;
    gs_tSystickTimeLastUpdate.u32Ms = 0;
    
    gs_eSystickMode = systick_mode_run;
    gs_hTickUpdate = a_hTickUpdate;
    
    TIM2_DeInit();
    CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);         /* enable timer 4 clock */
    

    TIM2_TimeBaseInit(TIM2_Prescaler_64, TIM2_CounterMode_Down, SYSTICK_RUN_RELOAD_COUNTER);                           /* set prescaler = 64 */
    TIM2_SetAutoreload(SYSTICK_RUN_RELOAD_COUNTER);

    
    
    TIM2_ClearITPendingBit(TIM2_IT_Update);
    TIM2_ITConfig(TIM2_IT_Update, ENABLE);
    TIM2_Cmd(ENABLE); 
    
}   /* systick_Init() */


/***************************************************************************************************
 * @fn      systick_SetReloadTime()
 *
 * @brief   reload tick set, in run mode = 1ms, in sleep mode = 99.37ms
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_SetReloadTime( systick_mode a_eMode )
{
    systick_uint16 u16Counter = 0;
    systick_uint32 u32Time = 0; /* unit = 0.01ms */
    systick_mode eLastMode = gs_eSystickMode;
    
    
    //ENTER_CRITICAL_SECTION();
    
    
    
    //TIM2_DeInit();
    //CLK_PeripheralClockConfig(CLK_Peripheral_TIM2, ENABLE);         /* enable timer 4 clock */
    
    TIM2_ITConfig(TIM2_IT_Update, DISABLE);
    if( systick_mode_run == a_eMode )
    {
        /* read data passed */
        u16Counter = TIM2_GetCounter();
        TIM2_TimeBaseInit(TIM2_Prescaler_64, TIM2_CounterMode_Down, SYSTICK_RUN_RELOAD_COUNTER);                           /* set prescaler = 64 */
        TIM2_SetAutoreload(SYSTICK_RUN_RELOAD_COUNTER);
    }
    else
    {
        /* read data passed */
        u16Counter = TIM2_GetCounter();
        TIM2_TimeBaseInit(TIM2_Prescaler_64, TIM2_CounterMode_Down, SYSTICK_SLEEP_RELOAD_COUNTER);                           /* set prescaler = 64 */
        TIM2_SetAutoreload(SYSTICK_SLEEP_RELOAD_COUNTER);
    }
    gs_eSystickMode = a_eMode;
    TIM2_ClearITPendingBit(TIM2_IT_Update);
    TIM2_ITConfig(TIM2_IT_Update, ENABLE);
    
    
    /* update tick */
    if( systick_mode_run == eLastMode )
    {
        u16Counter = SYSTICK_RUN_RELOAD_COUNTER - u16Counter;
        u32Time = u16Counter * 100000 / SYSTICK_RUN_TIMER_CLK;
    }
    else
    {
        u16Counter = SYSTICK_SLEEP_RELOAD_COUNTER - u16Counter;
        u32Time = u16Counter * 100000 / SYSTICK_SLEEP_TIMER_CLK;
    }
    
    //TIM2_Cmd(ENABLE); 
    
    
    
    u32Time += gs_tSystickTime.u16delta;
    gs_tSystickTime.u32Ms += (u32Time / 100);
    gs_tSystickTime.u16delta = u32Time % 100;
    
    if(systick_mode_run == a_eMode)
    {
        if(gs_hTickUpdate)
        {
            ENTER_CRITICAL_SECTION();
            u16Counter = gs_tSystickTimeLastUpdate.u32Ms <= gs_tSystickTime.u32Ms ? \
                gs_tSystickTime.u32Ms - gs_tSystickTimeLastUpdate.u32Ms : \
                    0xFFFFFFFF - gs_tSystickTimeLastUpdate.u32Ms + gs_tSystickTime.u32Ms + 1;
            gs_tSystickTimeLastUpdate.u32Ms = gs_tSystickTime.u32Ms;
            EXIT_CRITICAL_SECTION();
            gs_hTickUpdate(u16Counter);
            
        }
    }
    

    /* set data */
    //EXIT_CRITICAL_SECTION();
}   /* systick_SetReloadTime() */


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
    systick_uint32 u32Time = 0;
    
    if(gs_eSystickMode == systick_mode_run)
    {
        gs_tSystickTime.u32Ms ++;
        if(gs_hTickUpdate)
        {
            gs_hTickUpdate(1);
            gs_tSystickTimeLastUpdate.u32Ms = gs_tSystickTime.u32Ms;
        }
    }
    else
    {
        u32Time = (SYSTICK_SLEEP_RELOAD_COUNTER + 1) * 100000 / SYSTICK_SLEEP_TIMER_CLK;
        u32Time += gs_tSystickTime.u16delta;
        gs_tSystickTime.u32Ms += (u32Time / 100);
        gs_tSystickTime.u16delta = u32Time % 100;
    }
    
    if(gs_tSystickTime.u32Ms > 600000*3)
    {
        gs_tSystickTime.u32Ms = 0;
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



/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
 

#endif 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/