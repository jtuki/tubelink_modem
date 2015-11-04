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
#define SYSTICK_LPTIME_CLOCK_PRESCALE       (LPTIM_PRESCALER_DIV32)        /* clock = 32768 / 32 = 1024 = 2^10 */
#define SYSTICK_LPTIME_CALC_TIME(__timeUnit)     ((systick_uint32)(__timeUnit) >> 10 )
#define SYSTICK_LPTIM_WAKE_RELOAD_VALUE             (2)
#define SYSTICK_LPTIM_WAKE_CMP_VALUE                (1)
#define SYSTICK_LPTIME_SLEEP_MAX_RELOAD_VALUE       (65535)
/* this value is set to generate a compare interrupt to wakeup cpu, in order to set wake reload value, beacuse reload value should set before reload */
#define SYSTICK_LPTIME_SLEEP_CMP_RELOAD_DELTA_VALUE          (10)
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
volatile static systickTime_t gs_tSystickTime;
static systick_uint32 gs_u32CurReloadTimeUnitCnt = 0;



/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
/***************************************************************************************************
 * @fn      systick_ClockInit__()
 *
 * @brief   init clock for lptim
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void systick_ClockInit__( void );



 
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
    
    
    systick_ClockInit__();
    
    
    
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
    gs_u32CurReloadTimeUnitCnt = SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT(SYSTICK_LPTIM_WAKE_RELOAD_VALUE + 1);
    
    
    
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
    __disable_irq();
    //HAL_LPTIM_PWM_Start_IT( &hlptim1, SYSTICK_LPTIM_WAKE_RELOAD_VALUE, SYSTICK_LPTIM_WAKE_CMP_VALUE );
    __HAL_LPTIM_COMPARE_SET( &hlptim1, SYSTICK_LPTIM_WAKE_CMP_VALUE );
    __HAL_LPTIM_AUTORELOAD_SET( &hlptim1, SYSTICK_LPTIM_WAKE_RELOAD_VALUE );
    gs_u32CurReloadTimeUnitCnt = SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT(SYSTICK_LPTIM_WAKE_RELOAD_VALUE + 1);
    
    while( (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_ARROK) ==RESET) || (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK) ==RESET) ){}
    
    //__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
    //__HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARRM);
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARROK);
    
    
    __enable_irq();
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
    
    __disable_irq();
    /* set pwm period */
    __HAL_LPTIM_AUTORELOAD_SET( &hlptim1, u32Temp );
    __HAL_LPTIM_COMPARE_SET( &hlptim1, u32Temp - SYSTICK_LPTIME_SLEEP_CMP_RELOAD_DELTA_VALUE );
    
    
    
    /* wait for cpmok flag */
    while( (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_ARROK) ==RESET) || (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK) ==RESET) ){}
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARROK);
    __enable_irq();

    
    


}

unsigned int u32WakeTime = 0;
unsigned int u32WakeTime1 = 0;

/***************************************************************************************************
 * @fn      systick_setReloadAfterStopWake()
 *
 * @brief   set reload value after wakeup from stop mode
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_setReloadAfterStopWake( void )
{
#if 1
    systick_uint32 u32Count = 0;
    systick_uint32 u32OverFlowCount = 0;   /* overflow count of reload count = count - reload */
    const systick_uint32 u32Delta = 480;  /* delta for process delay */

    __disable_irq();
    u32Count = HAL_LPTIM_ReadCounter( &hlptim1 );
    
    /* when set reload, it will interrupt soon */
    if( u32Count >= (SYSTICK_LPTIM_WAKE_RELOAD_VALUE + 1) ){
        u32OverFlowCount = u32Count - SYSTICK_LPTIM_WAKE_RELOAD_VALUE;
        u32WakeTime1 ++;
    }else{
        u32OverFlowCount = 0;
    }
    u32WakeTime ++;
     /* set new reload value */
    __HAL_LPTIM_COMPARE_SET( &hlptim1, SYSTICK_LPTIM_WAKE_CMP_VALUE );
    __HAL_LPTIM_AUTORELOAD_SET( &hlptim1, SYSTICK_LPTIM_WAKE_RELOAD_VALUE );
    /* wait write active */
    while( (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_ARROK) ==RESET) || (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK) ==RESET) ){}
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARROK);
    
    
    gs_tSystickTime.u32delta += SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT(u32OverFlowCount) + u32Delta;
    
    gs_tSystickTime.u32Ms += SYSTICK_LPTIME_CALC_TIME( gs_tSystickTime.u32delta );
    gs_tSystickTime.u32delta &= 0x03FF;

    //systick_SetWakeReload();
    
    __enable_irq();
    /* add tick */
#endif

#if 0
    systick_uint32 u32Count = 0;
    const systick_uint32 u32Delta = 0;  /* delta for process delay */
    
    __disable_irq();
    u32Count = HAL_LPTIM_ReadCounter( &hlptim1 );
    
    /* set new reload value */
    __HAL_LPTIM_COMPARE_SET( &hlptim1, SYSTICK_LPTIM_WAKE_CMP_VALUE );
    __HAL_LPTIM_AUTORELOAD_SET( &hlptim1, SYSTICK_LPTIM_WAKE_RELOAD_VALUE );
    /* wait write active */
    while( (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_ARROK) ==RESET) || (__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_CMPOK) ==RESET) ){}
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPOK);
    __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARROK);

    if( u32Count >= SYSTICK_LPTIM_WAKE_RELOAD_VALUE ){
        while(__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_ARRM) ==RESET){
        }
        __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
        __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARRM);
        u32Count += 1;

    }else if( (SYSTICK_LPTIM_WAKE_RELOAD_VALUE - u32Count) < 2 ){   /*  */
        while(__HAL_LPTIM_GET_FLAG(&hlptim1, LPTIM_FLAG_ARRM) ==RESET){
        }

        __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_CMPM);
        __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARRM);
        u32Count = SYSTICK_LPTIM_WAKE_RELOAD_VALUE + 1;
    }else{
        u32Count = 0;
    }

    __enable_irq();

    
    
    __disable_irq();
    gs_tSystickTime.u32delta += SYSTICK_LPTIME_CALC_RELOAD_TO_TIMEUNIT_CNT(u32Count) + u32Delta;
    
    gs_tSystickTime.u32Ms += SYSTICK_LPTIME_CALC_TIME( gs_tSystickTime.u32delta );
    gs_tSystickTime.u32delta &= 0x03FF;
    __enable_irq();

#endif
    
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


/***************************************************************************************************
 * @fn      systick_ClockInit__()
 *
 * @brief   init clock for lptim
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void systick_ClockInit__( void )
{
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    /* Enable MSI Oscillator */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    while( HAL_TIMEOUT == HAL_RCC_OscConfig(&RCC_OscInitStruct)){
        ;
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPTIM1;
    PeriphClkInit.LptimClockSelection = RCC_LPTIM1CLKSOURCE_LSE;  
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);

}

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
    systick_uint32 temp = 0;

    if( hlptim->Instance == LPTIM1 ){
        gs_tSystickTime.u32delta += gs_u32CurReloadTimeUnitCnt;
        
        temp = SYSTICK_LPTIME_CALC_TIME( gs_tSystickTime.u32delta );
        
        gs_tSystickTime.u32Ms += temp;
        
        gs_tSystickTime.u32delta &= 0x03FF;
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
