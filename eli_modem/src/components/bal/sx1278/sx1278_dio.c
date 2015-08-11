/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    sx1278_dio.c
* @data         2015/07/07
* @auther       chuanpengl
* @module       sx1278 dio
* @brief        sx1278 dio
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "sx1278_dio.h"

#if (BAL_USE_SX1278 == BAL_MODULE_ON)

#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
#include "stm32l0xx_hal.h"
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
/*!
 * SX1276 RESET I/O definitions
 */
#define RESET_IOPORT                                GPIOA
#define RESET_PIN                                   GPIO_PIN_15
#define RESET_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

/*!
 * SX1276 DIO pins  I/O definitions
 */
#define DIO0_IOPORT                                 GPIOA
#define DIO0_PIN                                    GPIO_PIN_12
#define DIO0_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO1_IOPORT                                 GPIOA
#define DIO1_PIN                                    GPIO_PIN_11
#define DIO1_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO2_IOPORT                                 GPIOA
#define DIO2_PIN                                    GPIO_PIN_10
#define DIO2_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO3_IOPORT                                 GPIOA
#define DIO3_PIN                                    GPIO_PIN_9
#define DIO3_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO4_IOPORT                                 GPIOA
#define DIO4_PIN                                    GPIO_PIN_8
#define DIO4_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#define DIO5_IOPORT                                 GPIOB
#define DIO5_PIN                                    GPIO_PIN_2
#define DIO5_CLK_ENABLE()                          __GPIOB_CLK_ENABLE()


#define CTRL1_IOPORT                                 GPIOB
#define CTRL1_PIN                                    GPIO_PIN_4
#define CTRL1_CLK_ENABLE()                          __GPIOB_CLK_ENABLE()

#define CTRL2_IOPORT                                 GPIOA
#define CTRL2_PIN                                    GPIO_PIN_4
#define CTRL2_CLK_ENABLE()                          __GPIOA_CLK_ENABLE()

#endif      /* ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 ) */


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
 * @fn      sx1278_DioDelay__()
 *
 * @brief   sx1278 dio de init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void sx1278_DioDelay__(rfUint8 a_u8DelayCnt);

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
 * @fn      sx1278_DioInit()
 *
 * @brief   sx1278 dio init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_DioInit(void)
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    GPIO_InitTypeDef tGpioInit;

    tGpioInit.Mode      = GPIO_MODE_INPUT;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;

    /* dio 0 init as input */
    DIO0_CLK_ENABLE();
    tGpioInit.Pin       = DIO0_PIN;
    HAL_GPIO_Init(DIO0_IOPORT, &tGpioInit);
    
    /* dio 1 init as input */
    DIO1_CLK_ENABLE();
    tGpioInit.Pin       = DIO1_PIN;
    HAL_GPIO_Init(DIO1_IOPORT, &tGpioInit);
    
    /* dio 2 init as input */
    DIO2_CLK_ENABLE();
    tGpioInit.Pin       = DIO2_PIN;
    HAL_GPIO_Init(DIO2_IOPORT, &tGpioInit);
    
    /* dio 3 init as input */
    DIO3_CLK_ENABLE();
    tGpioInit.Pin       = DIO3_PIN;
    HAL_GPIO_Init(DIO3_IOPORT, &tGpioInit);
    
    /* dio 4 init as input */
    DIO4_CLK_ENABLE();
    tGpioInit.Pin       = DIO4_PIN;
    HAL_GPIO_Init(DIO4_IOPORT, &tGpioInit);
    
    /* dio 5 init as input */
    DIO5_CLK_ENABLE();
    tGpioInit.Pin       = DIO5_PIN;
    HAL_GPIO_Init(DIO5_IOPORT, &tGpioInit);
    
    
    tGpioInit.Mode      = GPIO_MODE_OUTPUT_PP;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;

    /* reset init as output */
    RESET_CLK_ENABLE();
    tGpioInit.Pin       = RESET_PIN;
    HAL_GPIO_Init(RESET_IOPORT, &tGpioInit);
    
    /* ctrl 1 init as output */
    CTRL1_CLK_ENABLE();
    tGpioInit.Pin       = CTRL1_PIN;
    HAL_GPIO_Init(CTRL1_IOPORT, &tGpioInit);
    
    /* ctrl 2 init as output */
    CTRL2_CLK_ENABLE();
    tGpioInit.Pin       = CTRL2_PIN;
    HAL_GPIO_Init(CTRL2_IOPORT, &tGpioInit);

#endif
}   /* sx1278_DioInit() */

/***************************************************************************************************
 * @fn      sx1278_DioDeInit()
 *
 * @brief   sx1278 dio de init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_DioDeInit(void)
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    HAL_GPIO_DeInit(RESET_IOPORT, RESET_PIN);
    HAL_GPIO_DeInit(DIO0_IOPORT, DIO0_PIN);
    HAL_GPIO_DeInit(DIO1_IOPORT, DIO1_PIN);
    HAL_GPIO_DeInit(DIO2_IOPORT, DIO2_PIN);
    HAL_GPIO_DeInit(DIO3_IOPORT, DIO3_PIN);
    HAL_GPIO_DeInit(DIO4_IOPORT, DIO4_PIN);
    HAL_GPIO_DeInit(DIO5_IOPORT, DIO5_PIN);
    HAL_GPIO_DeInit(CTRL1_IOPORT, CTRL1_PIN);
    HAL_GPIO_DeInit(CTRL2_IOPORT, CTRL2_PIN);
#endif
}   /* sx1278_DioDeInit() */


/***************************************************************************************************
 * @fn      sx1276_Dio2EnableInt()
 *
 * @brief   sx1278 dio enable interrupt dio 2
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1276_Dio2EnableInt( rfBool a_bEnable )
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    
    GPIO_InitTypeDef tGpioInit;
    

    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;
    tGpioInit.Pin       = DIO2_PIN;
    
    if(rfFalse == a_bEnable){
        tGpioInit.Mode = GPIO_MODE_INPUT;
        HAL_GPIO_Init(DIO2_IOPORT, &tGpioInit);
    }
    /* enable */
    else{
        tGpioInit.Mode = GPIO_MODE_IT_RISING;
        HAL_GPIO_Init(DIO2_IOPORT, &tGpioInit);
        HAL_NVIC_SetPriority(EXTI4_15_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);
    }
    
#endif
}   /* sx1276_Dio2EnableInt() */



/***************************************************************************************************
 * @fn      sx1278_DioReset()
 *
 * @brief   sx1278 dio reset
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_DioReset( rfBool a_bReset )
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    if( a_bReset == rfTrue ){
        HAL_GPIO_WritePin(RESET_IOPORT, RESET_PIN, GPIO_PIN_RESET); /* reset pin = low level */
    }else{
        HAL_GPIO_WritePin(RESET_IOPORT, RESET_PIN, GPIO_PIN_SET);   /* reset pin = high level */
    }
#endif
}

/***************************************************************************************************
 * @fn      sx1278_DioReadDio0()
 *
 * @brief   sx1278 dio read dio 0
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline rfUint8 sx1278_DioReadDio0( void )
{
#if(PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0)
  return HAL_GPIO_ReadPin(DIO0_IOPORT, DIO0_PIN) == GPIO_PIN_RESET ? 0 : 1;
#endif
}   /* sx1278_DioReadDio0() */

/***************************************************************************************************
 * @fn      sx1278_DioReadDio1()
 *
 * @brief   sx1278 dio read dio 1
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline uint8_t sx1278_DioReadDio1( void )
{
#if(PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0)
    return HAL_GPIO_ReadPin( DIO1_IOPORT, DIO1_PIN ) == GPIO_PIN_RESET ? 0 : 1;
#endif
}   /* sx1278_DioReadDio1() */

/***************************************************************************************************
 * @fn      sx1278_DioReadDio2()
 *
 * @brief   sx1278 dio read dio 2
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline uint8_t sx1278_DioReadDio2( void )
{
#if(PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0)
    return HAL_GPIO_ReadPin( DIO2_IOPORT, DIO2_PIN ) == GPIO_PIN_RESET ? 0 : 1;
#endif
}   /* sx1278_DioReadDio2() */

/***************************************************************************************************
 * @fn      sx1278_DioReadDio3()
 *
 * @brief   sx1278 dio read dio 3
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline uint8_t sx1278_DioReadDio3( void )
{
#if(PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0)
    return HAL_GPIO_ReadPin( DIO3_IOPORT, DIO3_PIN ) == GPIO_PIN_RESET ? 0 : 1;
#endif
}   /* sx1278_DioReadDio3() */

/***************************************************************************************************
 * @fn      sx1278_DioReadDio4()
 *
 * @brief   sx1278 dio read dio 4
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline uint8_t sx1278_DioReadDio4( void )
{
#if(PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0)
    return 0;                                                       /* no use in this application */
#endif
}   /* sx1278_DioReadDio4() */

/***************************************************************************************************
 * @fn      sx1278_DioReadDio5()
 *
 * @brief   sx1278 dio read dio 5
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline uint8_t sx1278_DioReadDio5( void )
{
#if(PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0)
    return HAL_GPIO_ReadPin( DIO5_IOPORT, DIO5_PIN ) == GPIO_PIN_RESET ? 0 : 1;
#endif
}   /* sx1278_DioReadDio5() */

/***************************************************************************************************
 * @fn      sx1278_DioSetDioCtrl()
 *
 * @brief   sx1278 dio read dio ctrl
 *
 * @author  chuanpengl
 *
 * @param   a_bTxEnable  - rfTrue, tx mode
 *                       - rfFalse, rx mode
 *
 * @return  none
 */
inline void sx1278_DioSetDioCtrl( rfBool a_bTxEnable )
{
    /* do not understand this function's use */
    
#if(PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0)
    if(rfFalse == a_bTxEnable)
    {
        HAL_GPIO_WritePin(CTRL1_IOPORT, CTRL1_PIN, GPIO_PIN_RESET);                     /* nss pin = low level */
        HAL_GPIO_WritePin(CTRL2_IOPORT, CTRL2_PIN, GPIO_PIN_SET);                    /* nss pin = high level */
    }
    else
    {
        HAL_GPIO_WritePin(CTRL1_IOPORT, CTRL1_PIN, GPIO_PIN_SET);                    /* nss pin = high level */
        HAL_GPIO_WritePin(CTRL2_IOPORT, CTRL2_PIN, GPIO_PIN_RESET);                     /* nss pin = low level */
    }
#endif
}   /* sx1278_DioSetDioCtrl() */

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
/***************************************************************************************************
 * @fn      sx1278_DioDelay__()
 *
 * @brief   sx1278 dio de init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_DioDelay__(rfUint8 a_u8DelayCnt)
{
    while(a_u8DelayCnt--){
        ;
    }
}   /* sx1278_DioDelay__() */

 #endif /* (BAL_USE_SX1278 == BAL_MODULE_ON) */
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/

