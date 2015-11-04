/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the agreemen-
 * ts you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    app_hostif.h
* @data         2015/04/13
* @auther       chuanpengl
* @module       host interface
* @brief        interface for host connect
***************************************************************************************************/

#ifndef APP_HOSTIF_INCLUDE__
#define APP_HOSTIF_INCLUDE__
/***************************************************************************************************
 * INCLUDES
 */
#include "uart/uart.h"

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

#define HOSTIF_UAR_BUF_DEFAULT_SIZE (128)
#define HOSTIF_UART_RX_BUF_SIZE     (HOSTIF_UAR_BUF_DEFAULT_SIZE)
#define HOSTIF_UART_TX_BUF_SIZE     (HOSTIF_UAR_BUF_DEFAULT_SIZE)
#define HOSTIF_UART_TX_MSG_MAXLEN   (HOSTIF_UAR_BUF_DEFAULT_SIZE)

/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */
typedef char hostIfChar;
typedef unsigned char hostIfUint8;


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
 */

 
/***************************************************************************************************
 * @fn      hostIf_Init()
 *
 * @brief   Initial of host interface
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_Init( void );

/***************************************************************************************************
 * @fn      hostIf_Run()
 *
 * @brief   host interface run, this should invoked with 5ms
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  uartFalse - uart busy, else uart idle
 */
uartBool hostIf_Run( void );

/***************************************************************************************************
 * @fn      hostIf_SendToHost()
 *
 * @brief   Initial of host interface
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_SendToHost( hostIfChar *a_pu8Data, hostIfUint8 a_u8Length );
#ifdef __cplusplus
}
#endif

#endif /* APP_HOSTIF_INCLUDE__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*   context: here write modified history
*
***************************************************************************************************/
