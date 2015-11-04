/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    uart.h
* @data         2015/07/08
* @auther       chuanpengl
* @module       uart module
* @brief        uart module
***************************************************************************************************/
#ifndef __UART_H__
#define __UART_H__
/***************************************************************************************************
 * INCLUDES
 */

#include "lib_util/cfifo.h"

//#if (APP_USE_DTU == APP_MODULE_ON)

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

#define UART_EVT_NONE               (0x00)
#define UART_EVT_RX_TIMEOUT         (0x01)
#define UART_EVT_RX_FIFO_FULL       (0x02)
#define UART_EVT_RX_FIFO_HFULL      (0x04)
#define UART_EVT_TX_FIFO_EMPTY      (0x10)

#define __UART_DISABLE_INT()

#define __UART_ENABLE_INT()
    
/***************************************************************************************************
 * TYPEDEFS
 */
typedef char uartChar;
typedef unsigned char uartU8;
typedef signed char uartI8;
typedef unsigned short uartU16;
typedef signed short uartI16;
typedef unsigned int uartU32;
typedef signed int uartI32;
typedef enum{uartFalse = 0, uartTrue = !uartFalse}uartBool;
#define uartNull     ((void*)0)

typedef struct tag_UartHandle_t uartHandle_t;
typedef void (*uart_InitHdl)( void *a_ptUart );
typedef void (*uart_DeInitHdl)( uartHandle_t *a_ptUart );
typedef uartBool (*uart_SendByteWithItHdl)( uartU8 a_u8Byte );
typedef uartU32 (*uart_GetTickHdl)( void );
typedef uartBool (*uart_isTxingHdl)( void );
typedef uartU8 (*uart_EventHdl)( uartHandle_t *a_ptUart, uartU8 a_u8Evt);

struct tag_UartHandle_t{
    uartBool bInitial;         /* it is initial state or not */
    void *phUart;                   /* this will be cast to type hal_uart use */
    fifoBase_t *ptTxFifo;
    fifoBase_t *ptRxFifo;
    uart_InitHdl hInit;
    uart_DeInitHdl hDeInit;
    uart_SendByteWithItHdl hSendByteWithIt;     /* transmit byte handle */
    uart_GetTickHdl hGetTick;       /* get tick */
    uart_EventHdl hEvt;             /* event export */
    /* parameter below no need to be inited */
    
    uartU8 u8RxInterval;  /* when rx stop time is bigger than it, we think a fram end */
    uartU8 u8RxTimeout;
    uartBool bRxMonitorTmo;  /* when true, should monitor rx timeout  */
    uartBool bTxFlag;       /* tx happend before export tx fifo empty */
    uartU8 u8Evt;           /* event flag */
};


//uartBool uart_ReceiveByte( uartHandle_t * a_ptUart )


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
 * @fn      uart_Init()
 *
 * @brief   uart interface init 
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void uart_Init( uartHandle_t *a_ptUart );

/***************************************************************************************************
 * @fn      uart_Poll()
 *
 * @brief   uart interface poll, this will trigger event handle when event exist
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  uartFalse - uart busy, else uart idle
 */
uartBool uart_Poll( uartHandle_t *a_ptUart );

/***************************************************************************************************
 * @fn      uart_GetSendByte()
 *
 * @brief   uart get send byte
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *          a_pu8Data  - get fifo data
 *
 * @return  uartFalse  - failed, uartTrue  - success
 */
uartBool uart_GetSendByte( uartHandle_t *a_ptUart, uartU8 *a_pu8Data );

/***************************************************************************************************
 * @fn      uart_SetReceiveByte()
 *
 * @brief   uart set receive byte
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *          a_pu8Data  - receive fifo data
 *
 * @return  uartFalse  - failed, uartTrue  - success
 */
uartBool uart_SetReceiveByte( uartHandle_t *a_ptUart, uartU8 a_pu8Data );

/***************************************************************************************************
 * @fn      uart_GetReceiveLength()
 *
 * @brief   uart get receive length
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  receive length
 */
uartU16 uart_GetReceiveLength( uartHandle_t *a_ptUart );

/***************************************************************************************************
 * @fn      uart_SendBytes()
 *
 * @brief   uart interface poll, this will trigger event handle when event exist
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  real receive bytes
 */
uartU8 uart_ReceiveBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU8 a_u8Length );


/***************************************************************************************************
 * @fn      uart_SendBytes()
 *
 * @brief   uart interface poll, this will trigger event handle when event exist
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  none
 */
uartBool uart_SendBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU8 a_u8Length );

#ifdef __cplusplus
}
#endif

//#endif  /* (APP_USE_DTU == APP_MODULE_ON) */

#endif /* __UART_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150708
*   context: here write modified history
*
***************************************************************************************************/
