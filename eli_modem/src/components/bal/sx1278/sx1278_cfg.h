/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    sx1278_cfg.h
* @data         2015/07/07
* @auther       chuanpengl
* @module       sx1278 config file
* @brief        config sx1278 bal for different platform
***************************************************************************************************/
#ifndef __SX1278_CFG_H__
#define __SX1278_CFG_H__

/***************************************************************************************************
 * INCLUDES
 */
#include "bal_cfg.h"

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
/* platform for board stm32_lora_v1.0_20150528 */
#define PLATFORM_SX1278_STM32_LORA_V1_0         (0x01)

#define PLATFORM_SX1278_SELECT                  PLATFORM_SX1278_STM32_LORA_V1_0

/***************************************************************************************************
 * TYPEDEFS
 */
typedef char rfChar;
typedef unsigned char rfUint8;
typedef signed char rfInt8;
typedef unsigned short rfUint16;
typedef signed short rfInt16;
typedef unsigned int rfUint32;
typedef signed int rfInt32;
typedef enum{rfFalse = 0, rfTrue = !rfFalse}rfBool;
#define rfNull      ((void*)0)


/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */


#ifdef __cplusplus
}
#endif

#endif  /* (BAL_USE_SX1278 == BAL_MODULE_ON) */

#endif /* __SX1278_CFG_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150707
*   context: here write modified history
*
***************************************************************************************************/
