/***************************************************************************************************
* @file name	sleep.c
* @data   		2015/04/17
* @auther   	chuanpengl
* @module   	sleep module
* @brief 		sleep module
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "sleep.h"
#include "stm8l15x.h"
#include "systick\hal_systick.h"

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
 * @fn      slp_ChangeClk__()
 *
 * @brief   change clock
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline static void slp_ChangeClk__( SlpClk_e a_eClkType );

 /***************************************************************************************************
 * @fn      slp_UpdateTimeDuringSleep__()
 *
 * @brief   update time escape during sleep
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline static void slp_UpdateTimeDuringSleep__( void );

 /***************************************************************************************************
 * @fn      slp_GotoSleep__()
 *
 * @brief   goto sleep
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline static void slp_GotoSleep__( void );

 /***************************************************************************************************
 * @fn      slp_SetWakeupTimer__()
 *
 * @brief   set wakeup timer
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline static void slp_SetWakeupTimer__( slpUint16 a_u16WakeupTime );

 /***************************************************************************************************
 * @fn      slp_EnableWakeupTimer__()
 *
 * @brief   enable wakeup timer or not
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline static void slp_EnableWakeupTimer__( slpControlState_e a_eEnable );

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */


/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      slp_Request()
 *
 * @brief   request to sleep for some time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_Request( slpUint16 a_u16SleepTime )
{
    
    
    /* set wakeup time use rtc wakeup */
    if( a_u16SleepTime < 100 )
    {
        slp_SetWakeupTimer__( a_u16SleepTime );   /* set rtc wakeup timer */
    }
    
    /* change clock to lsi or lse */
    slp_ChangeClk__( SLP_CLK_LSI );
    
    /* set reload time */
    systick_SetReloadTime( systick_mode_sleep );
    
    if( a_u16SleepTime < 100 )
    {
        slp_EnableWakeupTimer__(slpEnable);
    }
    
    
    
    slp_GotoSleep__();/* sleep */

    slp_EnableWakeupTimer__(slpDisable);
    /* enter critical */
    ENTER_CRITICAL_SECTION();
    
    /* update time */
    slp_UpdateTimeDuringSleep__();
    
    /* change clock to hsi */
    slp_ChangeClk__( SLP_CLK_HSI );
   
    
    /* set timer 1 ms interrupt */
    systick_SetReloadTime( systick_mode_run );
    
    /* exit critical */
    EXIT_CRITICAL_SECTION();
}   /* slp_Request() */

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
 /***************************************************************************************************
 * @fn      slp_ChangeClk__()
 *
 * @brief   change clock
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_ChangeClk__( SlpClk_e a_eClkType )
{
    switch(a_eClkType)
    {
    case SLP_CLK_HSI:
        CLK_HSICmd(ENABLE); /* enable clock */
        while (((CLK->ICKCR)& 0x02)!=0x02); /* wait clock stabilized */
        CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); /* set prescale */
        CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_HSI); /* set clock */
        CLK_SYSCLKSourceSwitchCmd(ENABLE);/* switch clock */
        while (((CLK->SWCR)& 0x01)==0x01); /* wait clock stabilized */
        CLK_LSICmd(DISABLE); /* disable clock */
        
        break;
    case SLP_CLK_LSI:
        CLK_LSICmd(ENABLE); /* enable clock */
        while (((CLK->ICKCR)& 0x08)!=0x08); /* wait clock stabilized */
        CLK_SYSCLKDivConfig(CLK_SYSCLKDiv_1); /* set prescale */
        CLK_SYSCLKSourceConfig(CLK_SYSCLKSource_LSI); /* set clock */
        CLK_SYSCLKSourceSwitchCmd(ENABLE);/* switch clock */
        while (((CLK->SWCR)& 0x01)==0x01); /* wait clock stabilized */
        CLK_HSICmd(DISABLE); /* disable clock */
        break;
    case SLP_CLK_HSE:
        break;
    case SLP_CLK_LSE:
        break;
    default:
        break;
    }
}   /* slp_UpdateTimeDuringSleep__() */



 /***************************************************************************************************
 * @fn      slp_UpdateTimeDuringSleep__()
 *
 * @brief   update time escape during sleep
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_UpdateTimeDuringSleep__( void )
{
    ;
}   /* slp_UpdateTimeDuringSleep__() */


 /***************************************************************************************************
 * @fn      slp_GotoSleep__()
 *
 * @brief   goto sleep
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_GotoSleep__( void )
{
    /* cpu wake up will slow */
    /* close MVR */
    //CLK->ICKCR |= 0x10;

    /*  close ULP，set FWU */
    //PWR->CSR1 |= 0x6;

    /* halt */
    //halt();
    wfi();
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
}   /* slp_GotoSleep__() */


 /***************************************************************************************************
 * @fn      slp_SetWakeupTimer__()
 *
 * @brief   set wakeup timer
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_SetWakeupTimer__( slpUint16 a_u16WakeupTime )
{
    /* RTC will wake-up from halt every 10second*/
    RTC_DeInit();
    RTC_WakeUpCmd(DISABLE);
    CLK_PeripheralClockConfig(CLK_Peripheral_RTC, ENABLE);
    CLK_RTCClockConfig(CLK_RTCCLKSource_LSI, CLK_RTCCLKDiv_2);  /* lsi = 38kHz */
    
    RTC_WakeUpClockConfig(RTC_WakeUpClock_RTCCLK_Div16);    /* clk = 38/2/16 =  1187.5Hz */
    RTC_ClearITPendingBit(RTC_IT_WUT);
    RTC_ITConfig(RTC_IT_WUT, ENABLE);
    
    RTC_SetWakeUpCounter((slpUint16)(a_u16WakeupTime * WAKEUP_TICK_REEQ / 1000));
    //RTC_WakeUpCmd(ENABLE);
}   /* slp_SetWakeupTimer__() */

 /***************************************************************************************************
 * @fn      slp_EnableWakeupTimer__()
 *
 * @brief   enable wakeup timer or not
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_EnableWakeupTimer__( slpControlState_e a_eEnable )
{
    RTC_WakeUpCmd((FunctionalState)a_eEnable);
}   /* slp_EnableWakeupTimer__() */

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
