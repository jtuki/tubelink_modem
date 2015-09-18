/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    uart.c
* @data         2015/07/08
* @auther       chuanpengl
* @module       dtu module
* @brief        dtu module
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "uart.h"

//#if (APP_USE_DTU == APP_MODULE_ON)


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
 * LOCAL FUNCTIONS DECLEAR
 */

/***************************************************************************************************
 * @fn      uart_TimeToTimeout__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_u8Timeout  - time out value
 *          a_u8Now  - time now
 *
 * @return  none
 */
static uartI8 uart_TimeToTimeout__( uartU8 a_u8Timeout, uartU8 a_u8Now );

/***************************************************************************************************
 * @fn      uart_AssertParam__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_bCondition
 *
 * @return  none
 */
void uart_AssertParam__( uartBool a_bCondition );

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
void uart_Init( uartHandle_t *a_ptUart )
{
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->phUart) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hGetTick) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hDeInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hSendByteWithIt) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptRxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptTxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hEvt) );

    a_ptUart->bRxMonitorTmo = uartFalse;
    a_ptUart->bTxFlag = uartFalse;
    a_ptUart->u8Evt = UART_EVT_NONE;
    
    a_ptUart->hInit( a_ptUart->phUart );
    a_ptUart->bInitial = uartTrue;

}

/***************************************************************************************************
 * @fn      uart_Poll()
 *
 * @brief   uart interface poll, this will trigger event handle when event exist
 *
 * @author  chuanpengl
 *
 * @param   a_ptUart  - uart handle
 *
 * @return  none
 */
void uart_Poll( uartHandle_t *a_ptUart )
{
    uartU8 u8Evt = UART_EVT_NONE;

    uart_AssertParam__( (uartBool)(uartNull != a_ptUart) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->phUart) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hGetTick) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hDeInit) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hSendByteWithIt) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptRxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->ptTxFifo) );
    uart_AssertParam__( (uartBool)(uartNull != a_ptUart->hEvt) );
    
    if( a_ptUart->bInitial != uartTrue ){
        return;
    }

    if( fifoTrue == fifo_isFull(a_ptUart->ptRxFifo) ){
        u8Evt |= UART_EVT_RX_FIFO_FULL;
    }else{
        if( fifo_Size( a_ptUart->ptRxFifo ) > (fifo_Capacity( a_ptUart->ptRxFifo ) >> 1 )){
            u8Evt |= UART_EVT_RX_FIFO_HFULL;
        }
    }
    
    if( uartTrue == a_ptUart->bRxMonitorTmo ){
        a_ptUart->bRxMonitorTmo = uartFalse;
        if( 0 >= uart_TimeToTimeout__( a_ptUart->u8RxTimeout, (uartU8)(a_ptUart->hGetTick() & 0xFF) )){
            u8Evt |= UART_EVT_RX_TIMEOUT;
        }
    }
    
    if( uartTrue == a_ptUart->bTxFlag ){
        if( fifoTrue == fifo_isEmpty( a_ptUart->ptTxFifo ) ){
            a_ptUart->bTxFlag = uartFalse;
            u8Evt |= UART_EVT_TX_FIFO_EMPTY;
            }
    }
    
    
    if( u8Evt > 0 ){
        a_ptUart->hEvt( a_ptUart, u8Evt );
    }

}   /* uart_Poll() */



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
uartBool uart_GetSendByte( uartHandle_t *a_ptUart, uartU8 *a_pu8Data )
{
    if( a_ptUart->bInitial != uartTrue ){
        return uartFalse;
    }
    
    a_ptUart->bTxFlag = uartTrue;
    return (uartBool)fifo_Pop( a_ptUart->ptTxFifo, a_pu8Data );
}   /* uart_GetSendByte() */

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
uartBool uart_SetReceiveByte( uartHandle_t *a_ptUart, uartU8 a_u8Data )
{
    if( a_ptUart->bInitial != uartTrue ){
        return uartFalse;
    }
    
    a_ptUart->bRxMonitorTmo = uartTrue;
    a_ptUart->u8RxTimeout = (a_ptUart->hGetTick() & 0xFF) + a_ptUart->u8RxInterval;
    return (uartBool)fifo_Push( a_ptUart->ptRxFifo, a_u8Data );
}   /* uart_SetReceiveByte() */

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
uartU16 uart_GetReceiveLength( uartHandle_t *a_ptUart )
{
    if( a_ptUart->bInitial != uartTrue ){
        return 0;
    }
    return (uartU16)fifo_Size( a_ptUart->ptRxFifo );
}

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
uartU8 uart_ReceiveBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU8 a_u8Length )
{
    uartU8 u8RealReceBytes = a_u8Length;    /* real receive bytes equal request receive bytes */

    if( a_ptUart->bInitial != uartTrue ){
        return 0;
    }
    
    for( uartU8 i = 0; i < a_u8Length; i++ ){
        /* if fifo length is smaller than request length */
        if( fifoFalse == fifo_Pop( a_ptUart->ptRxFifo, &a_pu8Data[i] )){
            u8RealReceBytes = i + 1;        /* real receive bytes is fifo used size */
            break;
        }
    }
    return u8RealReceBytes;
}


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

// jt - used for profiling UART communication's performance.
// #include "kernel/timer.h"

uartBool uart_SendBytes( uartHandle_t *a_ptUart, uartU8 *a_pu8Data, uartU8 a_u8Length )
{
    uartBool bSuccess = uartFalse;
    uartU32 u32EmptySize = 0;
    uartU32 u32UseSize = 0;
    uartU8 u8Data;
    uartBool bTxRestart = uartFalse;
    
    // static struct time t1, t2, t3, t4; // jt - used for profiling UART communication's performance.

    if( a_ptUart->bInitial != uartTrue ){
        return uartFalse;
    }
    
    __UART_DISABLE_INT();
    
    u32UseSize = fifo_Size( a_ptUart->ptTxFifo );
    
    u32EmptySize = fifo_Capacity( a_ptUart->ptTxFifo );
    
    if( u32UseSize == 0 ){
        bTxRestart = uartTrue;
    }
    
    if( u32EmptySize >= a_u8Length ){
        fifo_Push( a_ptUart->ptTxFifo, *a_pu8Data );
        bSuccess = uartTrue;
    }
    __UART_ENABLE_INT();

    /* first byte has pushed */
    for( uartU8 i = 1; i < a_u8Length; i++ ) {
        fifo_Push( a_ptUart->ptTxFifo, a_pu8Data[i] );
    }


    if( uartTrue == bTxRestart ){
        // haddock_get_time_tick_now(&t1);
        uart_GetSendByte( a_ptUart, &u8Data );
        // haddock_get_time_tick_now(&t2);
        a_ptUart->hSendByteWithIt( u8Data ); /** \sa hostIf_SendByteWithIt__() */
        // haddock_get_time_tick_now(&t3);
    }

    return uartTrue;
}

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */





/***************************************************************************************************
 * @fn      dtu_UartIfPoll__()
 *
 * @brief   dtu uart interface poll, dtu will communicate with host through this interface
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void dtu_UartIfPoll__( void )
{
    #if 0
    os_EnterCritical();
    if( dtuTrue == gs_tUart.bRxTmoMonitor ){
        if(0 >= os_TaskTimeToTmo(gs_tUart.u32RxTimeout, os_GetTick()))
        {
            gs_tUart.bRxTmoMonitor = dtuFalse;
            
            /* timeout event */
            gs_u16DtuLoRaSendReqSize = fifo_Size(gs_tUart.ptRxFifo);
            gs_eDtuLoRaStReq = DTU_LORA_ST_TX;
        }
    }
    os_ExitCritical();
    #endif
}

/***************************************************************************************************
 * @fn      uart_TimeToTimeout__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_u8Timeout  - time out value
 *          a_u8Now  - time now
 *
 * @return  none
 */
uartI8 uart_TimeToTimeout__( uartU8 a_u8Timeout, uartU8 a_u8Now )
{
    uartI8 i8Delta = 0;
    uartU8 u8Delta = 0;
    
    
    
    u8Delta = a_u8Timeout >= a_u8Now ? a_u8Timeout - a_u8Now : a_u8Now - a_u8Timeout;
    
    if( u8Delta < 0x80 ){
        i8Delta = a_u8Timeout >= a_u8Now ? u8Delta : -u8Delta;
    }else{
        u8Delta = 0x100 - u8Delta;
        i8Delta = a_u8Timeout > a_u8Now ? -u8Delta : u8Delta;
    }
    
    return i8Delta;
}   /* uart_TimeToTimeout__() */


/***************************************************************************************************
 * @fn      uart_AssertParam__()
 *
 * @brief   uart assert
 *
 * @author  chuanpengl
 *
 * @param   a_bCondition
 *
 * @return  none
 */
void uart_AssertParam__( uartBool a_bCondition )
{
    while( uartFalse == a_bCondition ){
    }
}   /* uart_AssertParam__() */



/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150708
*   context: here write modified history
*
***************************************************************************************************/
