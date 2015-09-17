/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    sx1278_spi.h
* @data         2015/07/07
* @auther       chuanpengl
* @module       sx1278 spi interface module
* @brief        spi interface
***************************************************************************************************/
#ifndef __SX1278_SPI_H__
#define __SX1278_SPI_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "sx1278_cfg.h"

#if (BAL_USE_SX1278 == BAL_MODULE_ON)

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */


/***************************************************************************************************
 * TYPEDEFS
 */


/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
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
void sx1278_SpiInit(void);

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
void sx1278_SpiDeInit(void);

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
void sx1278_SpiSetNssHigh( rf_bool a_bNssHigh );

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
void sx1278_SpiWrite( rf_uint8 *a_pu8Dat, rf_uint16 a_u16Size );

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
void sx1278_SpiRead( rf_uint8 *a_pu8Dat, rf_uint16 a_u16Size );


#ifdef __cplusplus
}
#endif

#endif /* (BAL_USE_SX1278 == BAL_MODULE_ON) */

#endif /* __SX1278_SPI_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150707
*   context: here write modified history
*
***************************************************************************************************/
