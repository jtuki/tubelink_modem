/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the agreemen-
 * ts you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    app_hostif.c
* @data         2015/04/13
* @auther       chuanpengl
* @module       host interface
* @brief        interface for host connect
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_hostif.h"
#include "uart/uart.h"
//#include "os.h"
#include "app_config.h"
#include "hal/hal_mcu.h"
#include <string.h>
// #include <stdio.h>

#if defined (MODEM_FOR_END_DEVICE) && MODEM_FOR_END_DEVICE == OS_TRUE
#include "external_comm_protocol/ecp_sn_modem.h"
#elif defined (MODEM_FOR_GATEWAY) && MODEM_FOR_GATEWAY == OS_TRUE
#include "external_comm_protocol/ecp_gw_modem.h"
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define ATCMD_IS_HEX(value)     (((value <= '9' && value >= '0') || (value <= 'F' && value >= 'A')\
     || (value <= 'f' && value >= 'a')) ? 1 : 0)

// jt added, to avoid compile warnings.
#define halUart_Write(a,b,c)

/***************************************************************************************************
 * TYPEDEFS
 */
typedef void (*atResponse_Hdl)(hostIfChar *a_pcCmd, hostIfUint8 a_u8Length);
typedef struct
{
    hostIfUint8 *pu8AtCmd;
    atResponse_Hdl hAtResp;
}atCmd_t;

FIFO_DEF(dtuTxFifo_t, HOSTIF_UART_TX_BUF_SIZE)
FIFO_DEF(dtuRxFifo_t, HOSTIF_UART_RX_BUF_SIZE)

/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */


/***************************************************************************************************
 * @fn      hostIf_AtParse__()
 *
 * @brief   at command parse
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtParse__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtParseCmd__()
 *
 * @brief   parse command and call response
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtParseCmd__ (const atCmd_t *a_ptAtCmdList, hostIfUint8 a_u8CmdCnt, \
    hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseAt__()
 *
 * @brief   at command response to command "AT\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseAt__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseCGMI__()
 *
 * @brief   at command response to command "AT+CGMI=?\r\n" or "AT+CGMI\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseCGMI__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseCGDT__()
 *
 * @brief   at command response to command "AT+CGDT=?\r\n" or "AT+CGDT\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseCGDT__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseCGSN__()
 *
 * @brief   at command response to command "AT+CGSN=?\r\n" or "AT+CGSN\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseCGSN__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseMAC__()
 *
 * @brief   at command response to command "AT+MAC=?\r\n" or "AT+MAC\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseMAC__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseSADDR__()
 *
 * @brief   at command response to command "AT+SADDR=?\r\n" or "AT+SADDR\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseSADDR__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseVer__()
 *
 * @brief   at command response to command "AT+VER=?\r\n" or "AT+VER\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseVer__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_AtResponseWrite__()
 *
 * @brief   at command response to command "AT+WRITE=?\r\n" or "AT+WRITE\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_AtResponseWrite__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length);

/***************************************************************************************************
 * @fn      hostIf_UartInit__()
 *
 * @brief   uart interface init, dtu will communicate with host through this interface
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_UartInit__( void *a_ptUartHdl );


/***************************************************************************************************
 * @fn      hostIf_UartDeInit__()
 *
 * @brief   uart interface deinit
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_UartDeInit__( uartHandle_t *a_ptUart );

/***************************************************************************************************
 * @fn      hostIf_SendByteWithIt__()
 *
 * @brief   uart interface deinit
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static uartBool hostIf_SendByteWithIt__( uartU8 a_u8Byte );

/***************************************************************************************************
 * @fn      hostIf_UartEvent__()
 *
 * @brief   host uart event
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static uartU8 hostIf_UartEvent__ (  uartHandle_t *a_ptUart, uartU8 a_u8Evt );

/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */

UART_HandleTypeDef UartHandle;
dtuTxFifo_t gs_tUartTxFifo;
dtuRxFifo_t gs_tUartRxFifo;
/* the two byte below is used for receive and send cach */
hostIfUint8 gs_u8ReceiveByte = 0;
hostIfUint8 gs_u8SendByte = 0;

uartHandle_t gs_tHostIfUart = 
{
    .bInitial = uartFalse,      /* this musb be init with false when define */
    .phUart = &UartHandle,                   /* this will be cast to type hal_uart use */
    .ptTxFifo = (fifoBase_t*)&gs_tUartTxFifo,
    .ptRxFifo = (fifoBase_t*)&gs_tUartRxFifo,
    .hInit = hostIf_UartInit__,
    .hDeInit = hostIf_UartDeInit__,
    .hSendByteWithIt = hostIf_SendByteWithIt__,     /* transmit byte handle */
    .hGetTick = (uart_GetTickHdl) systick_Get,       /* get tick */
    .hEvt = hostIf_UartEvent__,             /* event export */
    .u8RxInterval = 5, // jt - initialize the rx interval to 5ms
};


const static atCmd_t gsc_atAtCmd[] = 
{
    {"AT\r\n", hostIf_AtResponseAt__},
    
};

const static atCmd_t gsc_atAtCmdC[] = 
{
    {"AT+CGMI", hostIf_AtResponseCGMI__},
    {"AT+CGDT", hostIf_AtResponseCGDT__},
    {"AT+CGSN", hostIf_AtResponseCGSN__},
    
};

const static atCmd_t gsc_atAtCmdM[] = 
{
    {"AT+MAC", hostIf_AtResponseMAC__},    
};

const static atCmd_t gsc_atAtCmdS[] = 
{
    {"AT+SADDR", hostIf_AtResponseSADDR__},    
};

const static atCmd_t gsc_atAtCmdV[] = 
{
    {"AT+VER", hostIf_AtResponseVer__},    
};

const static atCmd_t gsc_atAtCmdW[] = 
{
    {"AT+WRITE", hostIf_AtResponseWrite__},    
};

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
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
void hostIf_Init( void )
{
    /* must init fifo first, otherwise, unknow error will occure */
    fifo_Init(gs_tHostIfUart.ptTxFifo, sizeof( gs_tUartTxFifo.au8Array ));
    fifo_Init(gs_tHostIfUart.ptRxFifo, sizeof( gs_tUartRxFifo.au8Array ));
    uart_Init( &gs_tHostIfUart );

} /* hostIf_Init() */


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
uartBool hostIf_Run( void )
{
#if 0
    static unsigned int i = 0;
    
    if( i++ > 100 ){
        i = 0;
        hostIf_SendToHost("hello", strlen("hello"));
    }
#endif
    
    return uart_Poll( &gs_tHostIfUart );
} /* hostIf_Run() */


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

// jt - used for profiling UART communication's performance.
// #include "kernel/timer.h"

void hostIf_SendToHost( hostIfChar *a_pu8Data, hostIfUint8 a_u8Length )
{
    // struct time t1, t2; // jt - used for profiling UART communication's performance.
    // haddock_get_time_tick_now(&t1);
    if(a_pu8Data && (a_u8Length < HOSTIF_UART_TX_MSG_MAXLEN) )
    {
        uart_SendBytes(&gs_tHostIfUart, (hostIfUint8*)a_pu8Data, a_u8Length);
    }
    // haddock_get_time_tick_now(&t2);
}   /* hostIf_SendToHost() */


/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */



/***************************************************************************************************
 * @fn      hostIf_AtParse__()
 *
 * @brief   at command parse
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtParse__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    halUart_Write(&gs_tHostIfUart, a_pcBuf, a_u8Length);
    if(a_u8Length >= 4)
    {
        if( ( 'A' == a_pcBuf[0] ) && ( 'T' == a_pcBuf[1] ) && \
            ( '\r' == a_pcBuf[a_u8Length - 2] ) && ( '\n' == a_pcBuf[a_u8Length - 1] ) )
        {
            if( 4 == a_u8Length )
            {
                gsc_atAtCmd[0].hAtResp(0,0);
            }
            else
            {
                if('+' == a_pcBuf[2])
                {
                    switch(a_pcBuf[3])
                    {
                    case 'C':
                        hostIf_AtParseCmd__(gsc_atAtCmdC, sizeof(gsc_atAtCmdC) / sizeof(atCmd_t),\
                            a_pcBuf, a_u8Length);
                        
                        break;
                    case 'M':
                        hostIf_AtParseCmd__(gsc_atAtCmdM, sizeof(gsc_atAtCmdM) / sizeof(atCmd_t),\
                            a_pcBuf, a_u8Length);
                        break;
                    case 'S':
                        hostIf_AtParseCmd__(gsc_atAtCmdS, sizeof(gsc_atAtCmdS) / sizeof(atCmd_t),\
                            a_pcBuf, a_u8Length);
                        break;
                    case 'V':
                        hostIf_AtParseCmd__(gsc_atAtCmdV, sizeof(gsc_atAtCmdV) / sizeof(atCmd_t),\
                            a_pcBuf, a_u8Length);
                        break;
                    case 'W':
                        hostIf_AtParseCmd__(gsc_atAtCmdW, sizeof(gsc_atAtCmdW) / sizeof(atCmd_t),\
                            a_pcBuf, a_u8Length);
                        break;
                    }
                }
                else    /* the third character is not '+' */
                {
                    ;
                }
            }
        }
    }

}   /* hostIf_AtParse__() */

/***************************************************************************************************
 * @fn      hostIf_AtParseCmd__()
 *
 * @brief   parse command and call response
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtParseCmd__ (const atCmd_t *a_ptAtCmdList, hostIfUint8 a_u8CmdCnt, \
    hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    hostIfUint8 u8Idx = 0;
    hostIfUint8 u8Length = 0;
    
    for( u8Idx = 0; u8Idx < a_u8CmdCnt; u8Idx ++)
    {
        u8Length = strlen((char const*)a_ptAtCmdList[u8Idx].pu8AtCmd);
        if( 0 == memcmp(a_pcBuf, a_ptAtCmdList[u8Idx].pu8AtCmd, u8Length ) )
        {
            a_ptAtCmdList[u8Idx].hAtResp(&a_pcBuf[u8Length], a_u8Length - u8Length);
        }
    }
}

/***************************************************************************************************
 * @fn      hostIf_AtResponseAt__()
 *
 * @brief   at command response to command "AT\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseAt__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
}/* hostIf_AtResponseAt__() */

/***************************************************************************************************
 * @fn      hostIf_AtResponseCGMI__()
 *
 * @brief   at command response to command "AT+CGMI=?\r\n" or "AT+CGMI\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseCGMI__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    if(a_pcBuf)
    {
        if(2 == a_u8Length)
        {
            halUart_Write(&gs_tHostIfUart, MANUFACTURER, strlen(MANUFACTURER));
            halUart_Write(&gs_tHostIfUart, "\r\nOK\r\n\r\n", strlen("\r\nOK\r\n\r\n"));
        }
        else if(('=' == a_pcBuf[0]) && ( '?' == a_pcBuf[1] ) && (4 == a_u8Length))
        {
            halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
        }
    }
}   /* hostIf_AtResponseCGMI__() */

/***************************************************************************************************
 * @fn      hostIf_AtResponseCGDT__()
 *
 * @brief   at command response to command "AT+CGDT=?\r\n" or "AT+CGDT\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseCGDT__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    if(a_pcBuf)
    {
        if(2 == a_u8Length)
        {
            halUart_Write(&gs_tHostIfUart, DEVICE_TYPE, strlen(DEVICE_TYPE));
            halUart_Write(&gs_tHostIfUart, "\r\nOK\r\n\r\n", strlen("\r\nOK\r\n\r\n"));
        }
        else if(('=' == a_pcBuf[0]) && ( '?' == a_pcBuf[1] ) && (4 == a_u8Length))
        {
            halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
        }
    }
}   /* hostIf_AtResponseCGDT__() */


/***************************************************************************************************
 * @fn      hostIf_AtResponseCGSN__()
 *
 * @brief   at command response to command "AT+CGSN=?\r\n" or "AT+CGSN\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseCGSN__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    hostIfChar au8SerialNumber[CFG_SN_LENGTH];
    hostIfUint8 u8Cnt = 0;
    
    if(a_pcBuf)
    {
        if(2 == a_u8Length)
        {
            cfg_GetSerialNumber(au8SerialNumber, CFG_SN_LENGTH);
            halUart_Write(&gs_tHostIfUart, "SN:", strlen("MAC:"));
            halUart_Write(&gs_tHostIfUart, au8SerialNumber, strlen(au8SerialNumber));
            halUart_Write(&gs_tHostIfUart, "\r\nOK\r\n\r\n", strlen("\r\nOK\r\n\r\n"));
        }
        else if(('=' == a_pcBuf[0]) && ( '?' == a_pcBuf[1] ) && (4 == a_u8Length))
        {
            halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
        }
        /* CFG_SN_LENGTH contain 16Byte SN character and a end character, so -1, +3 means '=' + '\r' + '\n' */
        else if(('=' == a_pcBuf[0]) && ((CFG_SN_LENGTH - 1 + 3) == a_u8Length))
        {
            /* check is mac address or not */
            for(u8Cnt = 1; u8Cnt < CFG_SN_LENGTH; u8Cnt ++)
            {
                if(0 == ATCMD_IS_HEX(a_pcBuf[u8Cnt]))
                {
                    halUart_Write(&gs_tHostIfUart, "ERROR\r\n\r\n", strlen("ERROR\r\n\r\n"));
                    break;
                }
            }
            if(u8Cnt >= CFG_SN_LENGTH)
            {
                halUart_Write(&gs_tHostIfUart, &a_pcBuf[1], CFG_SN_LENGTH);
                halUart_Write(&gs_tHostIfUart, "\r\nOK\r\n\r\n", strlen("\r\nOK\r\n\r\n"));
            }
        }
    }
}   /* hostIf_AtResponseCGSN__() */

/***************************************************************************************************
 * @fn      hostIf_AtResponseMAC__()
 *
 * @brief   at command response to command "AT+MAC=?\r\n" or "AT+MAC\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseMAC__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    hostIfChar au8Mac[CFG_MAC_LENGTH];
    hostIfUint8 u8Cnt = 0;
    
    if(a_pcBuf)
    {
        if(2 == a_u8Length)
        {
            cfg_GetMac(au8Mac, CFG_MAC_LENGTH);
            halUart_Write(&gs_tHostIfUart, "MAC:", strlen("MAC:"));
            halUart_Write(&gs_tHostIfUart, au8Mac, strlen(au8Mac));
            halUart_Write(&gs_tHostIfUart, "\r\nOK\r\n\r\n", strlen("\r\nOK\r\n\r\n"));
        }
        else if(('=' == a_pcBuf[0]) && ( '?' == a_pcBuf[1] ) && (4 == a_u8Length))
        {
            halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
        }
        /* CFG_MAC_LENGTH contain 12Byte Mac character and a end character, so -1, +3 means '=' + '\r' + '\n' */
        else if(('=' == a_pcBuf[0]) && ((CFG_MAC_LENGTH - 1 + 3) == a_u8Length))
        {
            /* check is mac address or not */
            for(u8Cnt = 1; u8Cnt < CFG_MAC_LENGTH; u8Cnt ++)
            {
                if(0 == ATCMD_IS_HEX(a_pcBuf[u8Cnt]))
                {
                    halUart_Write(&gs_tHostIfUart, "ERROR\r\n\r\n", strlen("ERROR\r\n\r\n"));
                    break;
                }
            }
            if(u8Cnt >= CFG_MAC_LENGTH)
            {
                halUart_Write(&gs_tHostIfUart, &a_pcBuf[1], CFG_MAC_LENGTH);
                halUart_Write(&gs_tHostIfUart, "\r\nOK\r\n\r\n", strlen("\r\nOK\r\n\r\n"));
            }
        }
    }
}   /* hostIf_AtResponseMAC__() */

/***************************************************************************************************
 * @fn      hostIf_AtResponseSADDR__()
 *
 * @brief   at command response to command "AT+SADDR=?\r\n" or "AT+SADDR\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseSADDR__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    if(a_pcBuf)
    {
        if(2 == a_u8Length)
        {
            halUart_Write(&gs_tHostIfUart, "SA:1234\r\nOK\r\n\r\n", strlen("SA:1234\r\nOK\r\n\r\n"));
        }
        else if(('=' == a_pcBuf[0]) && ( '?' == a_pcBuf[1] ) && (4 == a_u8Length))
        {
            halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
        }
    }
}   /* hostIf_AtResponseSADDR__() */

/***************************************************************************************************
 * @fn      hostIf_AtResponseVer__()
 *
 * @brief   at command response to command "AT+VER=?\r\n" or "AT+VER\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseVer__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    if(a_pcBuf)
    {
        if(2 == a_u8Length)
        {
            halUart_Write(&gs_tHostIfUart, HW_VERSION, strlen(HW_VERSION));
            halUart_Write(&gs_tHostIfUart, "\r\n", 2);
            halUart_Write(&gs_tHostIfUart, SW_VERSION, strlen(SW_VERSION));
            halUart_Write(&gs_tHostIfUart, "\r\nOK\r\n\r\n", strlen("\r\nOK\r\n\r\n"));
            
        }
        else if(('=' == a_pcBuf[0]) && ( '?' == a_pcBuf[1] ) && (4 == a_u8Length))
        {
            halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
        }
    }
}   /* hostIf_AtResponseVer__() */

/***************************************************************************************************
 * @fn      hostIf_AtResponseWrite__()
 *
 * @brief   at command response to command "AT+WRITE=?\r\n" or "AT+WRITE\r\n"
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_AtResponseWrite__ (hostIfChar *a_pcBuf, hostIfUint8 a_u8Length)
{
    hostIfUint8 u8Length = 0;
    
    if(a_pcBuf)
    {
        if(('=' == a_pcBuf[0]) && ( '?' == a_pcBuf[1] ) && (4 == a_u8Length))
        {
            halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
        }
        else
        {
            if(('=' == a_pcBuf[0]) && (',' == a_pcBuf[3]) && \
                ATCMD_IS_HEX(a_pcBuf[1]) && (ATCMD_IS_HEX(a_pcBuf[2]) ))
            {
                u8Length = (a_pcBuf[1] << 4) | a_pcBuf[2];
                u8Length = u8Length; //
                halUart_Write(&gs_tHostIfUart, "OK\r\n\r\n", strlen("OK\r\n\r\n"));
            }
            else
            {
                halUart_Write(&gs_tHostIfUart, "ERROR\r\n\r\n", strlen("ERROR\r\n\r\n"));
            }
        }
    }
}   /* hostIf_AtResponseWrite__() */



/***************************************************************************************************
 * @fn      hostIf_UartInit__()
 *
 * @brief   uart interface init, dtu will communicate with host through this interface
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_UartInit__( void *a_ptUartHdl )
{
    UART_HandleTypeDef *ptUart = (UART_HandleTypeDef *)a_ptUartHdl;

    GPIO_InitTypeDef  GPIO_InitStruct;

    /* Enable GPIO TX/RX clock */
    __GPIOA_CLK_ENABLE();
    
    /* UART TX GPIO pin configuration  */
    GPIO_InitStruct.Pin       = GPIO_PIN_2;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART2;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* UART RX GPIO pin configuration  */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Alternate = GPIO_AF4_USART2;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Enable USART2 clock */
    __USART2_CLK_ENABLE(); 
    
    ptUart->Instance = USART2;
    
    ptUart->Init.BaudRate   = 38400;
    ptUart->Init.WordLength = UART_WORDLENGTH_8B;
    ptUart->Init.StopBits   = UART_STOPBITS_1;
    ptUart->Init.Parity     = UART_PARITY_NONE;
    ptUart->Init.HwFlowCtl  = UART_HWCONTROL_NONE;
    ptUart->Init.Mode       = UART_MODE_TX_RX;
    ptUart->Init.OverSampling = UART_OVERSAMPLING_16;
    ptUart->Init.OneBitSampling = UART_ONEBIT_SAMPLING_DISABLED;

    while(HAL_UART_Init(&UartHandle) != HAL_OK)
    {
        ;/* if usart 2 init failed, loop until reset */
    }
    
    /* NVIC for USART1 */
    HAL_NVIC_SetPriority(USART2_IRQn, INT_PRIORITY_UART, 1);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    
    HAL_UART_Receive_IT(&UartHandle, &gs_u8ReceiveByte, 1);
}

/***************************************************************************************************
 * @fn      hostIf_UartDeInit__()
 *
 * @brief   uart interface deinit
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_UartDeInit__( uartHandle_t *a_ptUart )
{
    ;
}

/***************************************************************************************************
 * @fn      hostIf_SendByteWithIt__()
 *
 * @brief   uart interface deinit
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
uartBool hostIf_SendByteWithIt__( uartU8 a_u8Byte )
{
    gs_u8SendByte = a_u8Byte;
    HAL_UART_Transmit_IT(&UartHandle, &gs_u8SendByte, 1);
    return uartTrue;
}

/***************************************************************************************************
 * @fn      hostIf_UartEvent__()
 *
 * @brief   host uart event
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
uartU8 hostIf_UartEvent__ (  uartHandle_t *a_ptUart, uartU8 a_u8Evt )
{
    static hostIfUint8 au8Buf[HOSTIF_UART_RX_BUF_SIZE];
    hostIfUint8 u8ReceiveLength = 0;
    
    if( UART_EVT_RX_TIMEOUT == (a_u8Evt & UART_EVT_RX_TIMEOUT) ){
        u8ReceiveLength = uart_GetReceiveLength( a_ptUart );
        uart_ReceiveBytes( a_ptUart, au8Buf, u8ReceiveLength );

        /**
         * \note This is a dispatcher wrapper for both gateway and sensor-node's
         * modems.
         */
#if defined (MODEM_FOR_GATEWAY) && MODEM_FOR_GATEWAY == OS_TRUE
        ecp_gw_modem_dispatcher((os_uint8 *) au8Buf, (os_uint16) u8ReceiveLength);
#elif defined (MODEM_FOR_END_DEVICE) && MODEM_FOR_END_DEVICE == OS_TRUE
        ecp_sn_modem_dispatcher((os_uint8 *) au8Buf, (os_uint16) u8ReceiveLength);
#endif
        //hostIf_AtParse__(acBuf, halUart_Read(&gs_tHostIfUart, acBuf, sizeof(acBuf)));
    } else {
        if( UART_EVT_RX_FIFO_FULL == (a_u8Evt & UART_EVT_RX_FIFO_FULL) ){
            u8ReceiveLength = uart_GetReceiveLength( a_ptUart );
            uart_ReceiveBytes( a_ptUart, au8Buf, u8ReceiveLength );

            /**
             *  \note jt - we only pull the data, but perform nothing.
             * we don't handle such a long frame.
             */
        }
        
        if( UART_EVT_RX_FIFO_HFULL == (a_u8Evt & UART_EVT_RX_FIFO_HFULL) ){
            ;/* do nothing */
        }
    }
    
    if( UART_EVT_TX_FIFO_EMPTY == (a_u8Evt & UART_EVT_TX_FIFO_EMPTY) ){
        ;
    }

    return 0;
}   /* hostIf_UartCallBack__() */

/***************************************************************************************************
 * @fn      HAL_UART_TxCpltCallback()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if( USART2 == huart->Instance ){
        if( uartTrue == uart_GetSendByte( &gs_tHostIfUart, &gs_u8SendByte )){
            HAL_UART_Transmit_IT(&UartHandle, &gs_u8SendByte, 1);
        }
    }
}

/***************************************************************************************************
 * @fn      HAL_UART_RxCpltCallback()
 *
 * @brief   dtu uart tx cplt callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if( USART2 == huart->Instance ){
        uart_SetReceiveByte( &gs_tHostIfUart, gs_u8ReceiveByte );
        HAL_UART_Receive_IT(&UartHandle, &gs_u8ReceiveByte, 1);
    }

}

/***************************************************************************************************
 * @fn      HAL_UART_ErrorCallback()
 *
 * @brief   dtu uart error callback, this is weak in uart driver, so this should not modify
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if( USART2 == huart->Instance ){
      huart->ErrorCode = HAL_UART_ERROR_NONE;
      HAL_UART_Receive_IT(&UartHandle, &gs_u8ReceiveByte, 1);
    }
}

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
