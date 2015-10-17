/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    sx1278_spi.c
* @data         2015/07/07
* @auther       chuanpengl
* @module       sx1278 spi interface
* @brief        sx1278 spi interface define
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "sx1278_spi.h"

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
 * spi sck I/O definitions
 */
#define SCK_PORT                        GPIOA
#define SCK_PIN                         GPIO_PIN_5
#define SCK_CLK_ENABLE()                __GPIOA_CLK_ENABLE()
/*!
 * spi miso I/O definitions
 */
#define MISO_PORT                       GPIOA
#define MISO_PIN                        GPIO_PIN_6
#define MISO_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

/*!
 * spi mosi I/O definitions
 */
#define MOSI_PORT                       GPIOA
#define MOSI_PIN                        GPIO_PIN_7
#define MOSI_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

/*!
 * spi nss I/O definitions
 */
#define NSS_PORT                        GPIOA
#define NSS_PIN                         GPIO_PIN_4
#define NSS_CLK_ENABLE()                __GPIOA_CLK_ENABLE()

#endif

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
 * @fn      sx1278_SpiDelay__()
 *
 * @brief   sx1278 spi delay
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void sx1278_SpiDelay__( rf_uint16 a_u16DelayCnt );

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
static SPI_HandleTypeDef tSpiHdl;
#endif

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      sx1278_SpiInit()
 *
 * @brief   sx1278 spi port init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_SpiInit(void)
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    GPIO_InitTypeDef tGpioInit;
    
    tSpiHdl.Instance = SPI1;
    tSpiHdl.Instance               = SPI1;
    tSpiHdl.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
    tSpiHdl.Init.Direction         = SPI_DIRECTION_2LINES;
    tSpiHdl.Init.CLKPhase          = SPI_PHASE_1EDGE;
    tSpiHdl.Init.CLKPolarity       = SPI_POLARITY_LOW;
    tSpiHdl.Init.CRCCalculation    = SPI_CRCCALCULATION_DISABLED;
    tSpiHdl.Init.CRCPolynomial     = 7;
    tSpiHdl.Init.DataSize          = SPI_DATASIZE_8BIT;
    tSpiHdl.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    tSpiHdl.Init.NSS               = SPI_NSS_SOFT;
    tSpiHdl.Init.TIMode            = SPI_TIMODE_DISABLED;
    tSpiHdl.Init.Mode = SPI_MODE_MASTER;

    /* deinit spi port */
    HAL_SPI_DeInit(&tSpiHdl);
    /* Enable GPIO TX/RX clock */

    /* init spi port - sck */
    SCK_CLK_ENABLE();
    tGpioInit.Pin       = SCK_PIN;
    tGpioInit.Mode      = GPIO_MODE_AF_PP;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;
    tGpioInit.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(SCK_PORT, &tGpioInit);
    
    /* init spi port - mosi */
    MOSI_CLK_ENABLE();
    tGpioInit.Pin       = MOSI_PIN;
    tGpioInit.Pull      = GPIO_PULLUP;
    HAL_GPIO_Init(MOSI_PORT, &tGpioInit);
    
    /* init spi port - miso */
    MISO_CLK_ENABLE();
    tGpioInit.Pin       = MISO_PIN;
    tGpioInit.Pull      = GPIO_PULLUP;
    HAL_GPIO_Init(MISO_PORT, &tGpioInit);
    
    /* init spi port - nss */
    NSS_CLK_ENABLE();
    tGpioInit.Pin       = NSS_PIN;
    tGpioInit.Mode      = GPIO_MODE_OUTPUT_PP;
    tGpioInit.Pull      = GPIO_PULLUP;
    tGpioInit.Speed     = GPIO_SPEED_FAST;
    //tGpioInit.Alternate = GPIO_AF0_SPI1;
    HAL_GPIO_Init(NSS_PORT, &tGpioInit);
    
    sx1278_SpiSetNssHigh(rf_true);
    __SPI1_CLK_ENABLE();
    /* init spi port */
    if(HAL_SPI_Init(&tSpiHdl) != HAL_OK)
    {
        /* Initialization Error */
        //Error_Handler();
    }
    __HAL_SPI_ENABLE(&tSpiHdl);

#endif
}

/***************************************************************************************************
 * @fn      sx1278_SpiDeInit()
 *
 * @brief   sx1278 spi port de init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_SpiDeInit(void)
{
    HAL_SPI_DeInit(&tSpiHdl);
    HAL_GPIO_DeInit(NSS_PORT, NSS_PIN);
    HAL_GPIO_DeInit(SCK_PORT, SCK_PIN);
    HAL_GPIO_DeInit(MOSI_PORT, MOSI_PIN);
    HAL_GPIO_DeInit(MISO_PORT, MISO_PIN);
}


/***************************************************************************************************
 * @fn      sx1278_SpiSetNssHigh()
 *
 * @brief   sx1278 spi ctrl nss
 *
 * @author  chuanpengl
 *
 * @param   a_bNssHigh  - rfTrue, set high
 *                      - rfFalse, set low
 *
 * @return  none
 */
void sx1278_SpiSetNssHigh( rf_bool a_bNssHigh )
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    if( rf_false == a_bNssHigh ){
        
        HAL_GPIO_WritePin(NSS_PORT, NSS_PIN, GPIO_PIN_RESET);   /* nss pin = low level */
        sx1278_SpiDelay__(2);
    }else{
        sx1278_SpiDelay__(2);
        HAL_GPIO_WritePin(NSS_PORT, NSS_PIN, GPIO_PIN_SET);   /* nss pin = high level */
    }
#endif /* ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 ) */

}


/***************************************************************************************************
 * @fn      sx1278_SpiWrite()
 *
 * @brief   sx1278 spi write byte
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Dat  - data buffer for transmit
 *          a_u16Size  - data size for transmit
 *
 * @return  none
 */
void sx1278_SpiWrite( rf_uint8 *a_pu8Dat, rf_uint16 a_u16Size )
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    HAL_SPI_Transmit( &tSpiHdl, a_pu8Dat, a_u16Size, 0x1000 );
#endif /* ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 ) */
}

/***************************************************************************************************
 * @fn      sx1278_SpiRead()
 *
 * @brief   sx1278 spi read bytes
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Dat  - data buffer for receive
 *          a_u16Size  - data size for receive
 *
 * @return  none
 */
void sx1278_SpiRead( rf_uint8 *a_pu8Dat, rf_uint16 a_u16Size )
{
#if ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 )
    if( NULL != a_pu8Dat ){
        HAL_SPI_Receive( &tSpiHdl, a_pu8Dat, a_u16Size, 0x1000 );
    }
#endif /* ( PLATFORM_SX1278_SELECT == PLATFORM_SX1278_STM32_LORA_V1_0 ) */
}

/***************************************************************************************************
 * @fn      sx1278_SpiWriteRead()
 *
 * @brief   sx1278 spi write bytes and read bytes
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_SpiWriteRead(void)
{
}



/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */

/***************************************************************************************************
 * @fn      sx1278_SpiDelay__()
 *
 * @brief   sx1278 spi delay
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_SpiDelay__( rf_uint16 a_u16DelayCnt )
{
    while( a_u16DelayCnt -- > 0 ){
        __NOP();
    }
}

#endif /* (BAL_USE_SX1278 == BAL_MODULE_ON) */
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
