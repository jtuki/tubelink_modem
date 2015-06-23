/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the agreemen-
 * ts you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name	app_hostif.c
* @data   		2015/04/13
* @auther   	chuanpengl
* @module   	host interface
* @brief 		interface for host connect
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "app_hostif.h"
#include "uart\hal_uart.h"
//#include "os.h"
#include "app_config.h"

#include <string.h>
// #include <stdio.h>

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define ATCMD_IS_HEX(value)     (((value <= '9' && value >= '0') || (value <= 'F' && value >= 'A')\
     || (value <= 'f' && value >= 'a')) ? 1 : 0)

/***************************************************************************************************
 * TYPEDEFS
 */
typedef void (*atResponse_Hdl)(hostIfChar *a_pcCmd, hostIfUint8 a_u8Length);
typedef struct
{
    hostIfUint8 *pu8AtCmd;
    atResponse_Hdl hAtResp;
}atCmd_t;



/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */

/***************************************************************************************************
 * @fn      hostIf_UartCallBack__()
 *
 * @brief   host interface task
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void hostIf_UartCallBack__ (hostIfUint8 a_u8port, hostIfUint8 a_u8event);

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
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
static halUartChar gs_au8RxBuf[HOSTIF_UART_RX_BUF_SIZE];
static halUartChar gs_au8TxBuf[HOSTIF_UART_TX_BUF_SIZE];

static halUartCfg_t gs_tHostIfUart = 
{
    HAL_UART_PORT_0,        /* port */
    halUartFalse,           /* configured */
    HAL_UART_BR_115200,       /* baudRate */
    (halUartBool)HAL_UART_FLOW_OFF,      
                            /* flowControl */
    0,                      /* flowControlThreshold */
    10,                      /* idleTimeout = 3ms */
    {0, 0, HOSTIF_UART_RX_BUF_SIZE, gs_au8RxBuf},
                            /* rx */
    {0, 0, HOSTIF_UART_TX_BUF_SIZE, gs_au8TxBuf},
                            /* tx */
    halUartTrue,            /* intEnable */
    0,                      /* rxChRvdTime */
    /* callBackFunc */
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
    halUart_InitCfg(&gs_tHostIfUart);
    halUart_Init(&gs_tHostIfUart, hostIf_UartCallBack__);

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
 * @return  none
 */
void hostIf_Run( void )
{
    halUart_Poll(&gs_tHostIfUart);
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
void hostIf_SendToHost( hostIfChar *a_pu8Data, hostIfUint8 a_u8Length )
{
    // hostIfChar acBuf[16];
    
    if(a_pu8Data && (a_u8Length < HOSTIF_UART_TX_MSG_MAXLEN) )
    {
        //sprintf((char*)acBuf, "%s%02x,", "+Read:", a_u8Length);
        //halUart_Write(&gs_tHostIfUart, acBuf, strlen((char const *)acBuf));
        halUart_Write(&gs_tHostIfUart, a_pu8Data, a_u8Length);
        //halUart_Write(&gs_tHostIfUart, "\r\n", 2);
        
    }
}   /* hostIf_SendToHost() */


/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
/***************************************************************************************************
 * @fn      hostIf_UartCallBack__()
 *
 * @brief   host interface task
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void hostIf_UartCallBack__ (hostIfUint8 a_u8port, hostIfUint8 a_u8event)
{
    hostIfChar acBuf[32];
    switch(a_u8event)
    {
        case HAL_UART_RX_FULL:
        break;
        case HAL_UART_RX_ABOUT_FULL:
        break;
        case HAL_UART_RX_TIMEOUT:
            // jt todo - to save ROM space ...
            // hostIf_AtParse__(acBuf, halUart_Read(&gs_tHostIfUart, acBuf, sizeof(acBuf)));
        
        break;
        default:
        break;
    }
}   /* hostIf_UartCallBack__() */


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
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
