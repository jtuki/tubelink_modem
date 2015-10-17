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



#define SYSTICK_LPTIME_CLOCK_PRESCALE       (LPTIM_PRESCALER_DIV32)        /* clock = 32768 / 32 = 1024 = 2^10 */
#define SYSTICK_LPTIME_CALC_TIME(__timeUnit)     ((systick_uint32)(__timeUnit) >> 10 )
#define SYSTICK_LPTIM_WAKE_RELOAD_VALUE             (1)
#define SYSTICK_LPTIME_SLEEP_MAX_RELOAD_VALUE       (65535)
/* this value is set to generate a compare interrupt to wakeup cpu, in order to set wake reload value, beacuse reload value should set before reload */
#define SYSTICK_LPTIME_SLEEP_CMP_RELOAD_DELTA_VALUE          (5)    
#define SYSTICK_LPTIME_CALC_SLEEP_RELOAD_VALUE( __ms )      (((systick_uint32)(__ms)<<10)/1000)

/* when __reload = 1024, period is 1 second, 1 lptim count is 1/1024/1000 ms, here, time unit cnt is for ms unit */
#define SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT( __reload )      ( (__reload) * 1000 )


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
LPTIM_HandleTypeDef hlptim1;

/***************************************************************************************************
 * STATIC VARIABLES
 */
volatile static systickTime_t gs_tSystickTimeLastUpdate, gs_tSystickTime;
static systick_mode gs_eSystickMode;
static systickRemTick_t gs_tTickRem;
static systick_uint32 gs_u32SleepTime;
static systick_uint32 gs_u32CurReloadTimeUnitCnt = 0;
static systick_uint32 gs_u32LastReloadTimeUnitCnt = 0;



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
    gs_tSystickTime.u32Ms = 0;
    gs_tSystickTime.u32delta = 0;
    
    /* lptim1 init */
    hlptim1.Instance = LPTIM1;
    hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    hlptim1.Init.Clock.Prescaler = SYSTICK_LPTIME_CLOCK_PRESCALE;
    hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
    hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
    HAL_LPTIM_Init(&hlptim1);
    
    /* set pwm period */
    HAL_LPTIM_PWM_Start_IT( &hlptim1, SYSTICK_LPTIM_WAKE_RELOAD_VALUE, 0 );
    gs_u32LastReloadTimeUnitCnt = gs_u32CurReloadTimeUnitCnt = SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT(SYSTICK_LPTIM_WAKE_RELOAD_VALUE + 1);
    
    
    
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
    HAL_LPTIM_PWM_Start_IT( &hlptim1, SYSTICK_LPTIM_WAKE_RELOAD_VALUE, 0 );
    gs_u32CurReloadTimeUnitCnt = SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT(SYSTICK_LPTIM_WAKE_RELOAD_VALUE + 1);
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
    systick_uint32 u32Temp = SYSTICK_LPTIME_CALC_SLEEP_RELOAD_VALUE(a_u32Cnt) - 1;
    
    if(  u32Temp > SYSTICK_LPTIME_SLEEP_MAX_RELOAD_VALUE ){
        u32Temp = SYSTICK_LPTIME_SLEEP_MAX_RELOAD_VALUE;
    }
    
    /* set pwm period */
    HAL_LPTIM_PWM_Start_IT( &hlptim1, u32Temp, u32Temp - SYSTICK_LPTIME_SLEEP_CMP_RELOAD_DELTA_VALUE );
    
    gs_u32CurReloadTimeUnitCnt = SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT(u32Temp + 1);
    
    /* wait for last wake lptim reload match interrupt, after interrupt, cur value = last value, less then 2 ms */
    while( gs_u32LastReloadTimeUnitCnt != gs_u32CurReloadTimeUnitCnt ){
        ;
    }

}


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


void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef* hlptim)
{

    if(hlptim->Instance==LPTIM1)
    {
        /* USER CODE BEGIN LPTIM1_MspInit 0 */

        /* USER CODE END LPTIM1_MspInit 0 */
        /* Peripheral clock enable */
        __LPTIM1_CLK_ENABLE();
        /* Peripheral interrupt init*/
        HAL_NVIC_SetPriority(LPTIM1_IRQn, 0, 0);
        HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
        /* USER CODE BEGIN LPTIM1_MspInit 1 */

        /* USER CODE END LPTIM1_MspInit 1 */
    }

}

void HAL_LPTIM_MspDeInit(LPTIM_HandleTypeDef* hlptim)
{

    if(hlptim->Instance==LPTIM1)
    {
        /* USER CODE BEGIN LPTIM1_MspDeInit 0 */

        /* USER CODE END LPTIM1_MspDeInit 0 */
        /* Peripheral clock disable */
        __LPTIM1_CLK_DISABLE();

        /* Peripheral interrupt DeInit*/
        HAL_NVIC_DisableIRQ(LPTIM1_IRQn);

    }
    /* USER CODE BEGIN LPTIM1_MspDeInit 1 */

  /* USER CODE END LPTIM1_MspDeInit 1 */

}

void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim)/* HAL_LPTIM_AutoReloadMatchCallback */
{
    unsigned int temp = 0;

    if( hlptim->Instance == LPTIM1 ){
        gs_tSystickTime.u32delta += gs_u32LastReloadTimeUnitCnt;
        
        gs_u32LastReloadTimeUnitCnt = gs_u32CurReloadTimeUnitCnt;
        
        temp = SYSTICK_LPTIME_CALC_TIME( gs_tSystickTime.u32delta );
        
        gs_tSystickTime.u32Ms += temp;
        
        gs_tSystickTime.u32delta &= 0x03FF;
    }
}


void HAL_LPTIM_CompareMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    if( hlptim->Instance == LPTIM1 ){
        systick_SetWakeReload();
    }
}



void LPTIM1_IRQHandler(void)
{
    HAL_LPTIM_IRQHandler( &hlptim1 );
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
