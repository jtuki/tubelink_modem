/***************************************************************************************************
* @file name	hal_uart.h
* @data   		2015/03/29
* @auther   	chuanpengl
* @module   	hal uart module
* @brief 		hal_uart
***************************************************************************************************/
#ifndef __HAL_UART_H__
#define __HAL_UART_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "hal_board_cfg.h"

#if (1 == _SW_UART)

#include "assert.h"


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define halUartAssert               assert
#define halUartNull                 ((void*)0)
 
 
/***************************************************************************************************
 * CONSTANTS
 */
/***********************************
 * Serial Port Baudrate Settings
 * Have to match with baudrate table
 */
#define HAL_UART_BR_9600   0x00
#define HAL_UART_BR_19200  0x01
#define HAL_UART_BR_38400  0x02
#define HAL_UART_BR_57600  0x03
#define HAL_UART_BR_115200 0x04
#define HAL_UART_BR_END    0x05



/* Frame Format constant */

/* Stop Bits */
#define HAL_UART_ONE_STOP_BIT       0x00
#define HAL_UART_TWO_STOP_BITS      0x01

/* Parity settings */
#define HAL_UART_NO_PARITY          0x00
#define HAL_UART_EVEN_PARITY        0x01
#define HAL_UART_ODD_PARITY         0x02

/* Character Size */
#define HAL_UART_8_BITS_PER_CHAR    0x00
#define HAL_UART_9_BITS_PER_CHAR    0x01

/* Flow control */
#define HAL_UART_FLOW_OFF   0x00
#define HAL_UART_FLOW_ON    0x01
#define HAL_UART_FLOW_END   0x02

/* Ports */
#define HAL_UART_PORT_0   0x00
#define HAL_UART_PORT_1   0x01
#define HAL_UART_PORT_END 0x02

/* UART Status */
#define  HAL_UART_SUCCESS        0x00
#define  HAL_UART_UNCONFIGURED   0x01
#define  HAL_UART_NOT_SUPPORTED  0x02
#define  HAL_UART_MEM_FAIL       0x03
#define  HAL_UART_BAUDRATE_ERROR 0x04

/* UART Events */
#define HAL_UART_RX_FULL         0x01
#define HAL_UART_RX_ABOUT_FULL   0x02
#define HAL_UART_RX_TIMEOUT      0x04
#define HAL_UART_TX_FULL         0x08
#define HAL_UART_TX_EMPTY        0x10


/***************************************************************************************************
 * TYPEDEFS
 */
typedef unsigned char halUartBufLen_n;

typedef enum{halUartFalse = 0, halUartTrue = 1}halUartBool;
typedef char halUartChar;
typedef unsigned char halUartUint8;
typedef unsigned short halUartUint16;
typedef unsigned int halUartUint32;

typedef void (*halUartCallBack_t) (halUartUint8 a_u8port, halUartUint8 a_u8event);



typedef struct
{
  // The head or tail is updated by the Tx or Rx ISR respectively, when not polled.
  halUartBufLen_n   nBufHead;
  halUartBufLen_n   nBufTail;
  halUartBufLen_n   nBufSize;
  halUartChar *    pBuffer;
} halUartBufCtrl_t;

typedef struct
{
    halUartUint8        port;
    halUartBool         configured;
    halUartUint8        baudRate;
    halUartBool         flowControl;
    halUartUint16       flowControlThreshold;
    halUartUint8        idleTimeout;
    halUartBufCtrl_t    rx;
    halUartBufCtrl_t    tx;
    halUartBool         intEnable;
    halUartUint32       rxChRvdTime;
    halUartCallBack_t   callBackFunc;
}halUartCfg_t;



/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * EXTERNAL FUNCTIONS DECLEAR
 */
#define HALUART_INIT_CFG_PORT(a_ptUartCfg, a_u8Port)    \
        halUartAssert(halUartNull != a_ptUartCfg);\
        halUartAssert(HAL_UART_CNT >= a_u8Port);\
        a_ptUartCfg->port = a_u8Port;

#define HALUART_INIT_CFG_BAUDRATE(a_ptUartCfg, a_nBaudRate)     \
        halUartAssert(halUartNull != a_ptUartCfg);\
        halUartAssert(HAL_UART_BR_END > a_nBaudRate);\
        a_ptUartCfg->baudRate = a_nBaudRate;

/* do not support flow control on now */
#define HALUART_INIT_CFG_FLOWCTRL(a_ptUartCfg, a_nFlowCtrl)     \
        halUartAssert(halUartNull != a_ptUartCfg);\
        halUartAssert(HAL_UART_FLOW_END > a_nFlowCtrl);\
        a_ptUartCfg->flowControl = a_nFlowCtrl;
        
#define HALUART_INIT_CFG_IDLETMO(a_ptUartCfg, a_nTmoTick)     \
        halUartAssert(halUartNull != a_ptUartCfg);\
        halUartAssert(0 < a_nFlowCtrl);\
        a_ptUartCfg->idleTimeout = a_nTmoTick;
        
#define HALUART_INIT_CFG_RXBUF(a_ptUartCfg, a_pu8Buf, a_nSize)     \
        halUartAssert(halUartNull != a_ptUartCfg);\
        halUartAssert(halUartNull != a_pu8Buf);\
        a_ptUartCfg->rx.pBuffer = a_pu8Buf;\
        a_ptUartCfg->rx.nBufHead = 0;\
        a_ptUartCfg->rx.nBufTail = 0;\
        a_ptUartCfg->rx.nBufSize = a_nSize;

#define HALUART_INIT_CFG_TXBUF(a_ptUartCfg, a_pu8Buf, a_nSize)     \
        halUartAssert(halUartNull != a_ptUartCfg);\
        halUartAssert(halUartNull != a_pu8Buf);\
        a_ptUartCfg->tx.pBuffer = a_pu8Buf;\
        a_ptUartCfg->tx.nBufHead = 0;\
        a_ptUartCfg->tx.nBufTail = 0;\
        a_ptUartCfg->tx.nBufSize = a_nSize;
#endif

/***************************************************************************************************
 * @fn      halUart_InitCfg()
 *
 * @brief   Initial hal uart configuration
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_InitCfg( halUartCfg_t *a_ptUartCfg );

/***************************************************************************************************
 * @fn      halUart_Init()
 *
 * @brief   Initial hal uart
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_Init( halUartCfg_t *a_ptUartCfg, halUartCallBack_t a_hUartCB );

/***************************************************************************************************
 * @fn      halUart_DeInit()
 *
 * @brief   deinitialize hal uart
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_DeInit( halUartCfg_t *a_ptUartCfg );

/***************************************************************************************************
 * @fn      halUart_Sleep()
 *
 * @brief   sleep hal uart
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_Sleep( halUartCfg_t *a_ptUartCfg );

/***************************************************************************************************
 * @fn      halUart_Wakeup()
 *
 * @brief   wakeup hal uart
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_Wakeup( halUartCfg_t *a_ptUartCfg );

/***************************************************************************************************
 * @fn      halUart_Poll()
 *
 * @brief   poll hal uart
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_Poll( halUartCfg_t *a_ptUartCfg );

/***************************************************************************************************
 * @fn      halUart_Write()
 *
 * @brief   wait data to tx buffer, and start send automatic
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
halUartBufLen_n halUart_Write( halUartCfg_t *a_ptUartCfg, halUartChar *a_pcBuf, halUartBufLen_n a_nLen);

/***************************************************************************************************
 * @fn      halUart_Write()
 *
 * @brief   read data from rx buffer
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
halUartBufLen_n halUart_Read( halUartCfg_t *a_ptUartCfg, halUartChar *a_pcBuf, halUartBufLen_n a_nLen);

/***************************************************************************************************
 * @fn      halUart_GetTxAvailable()
 *
 * @brief   get available count of tx buffer
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
halUartBufLen_n halUart_GetTxAvailable( halUartCfg_t *a_ptUartCfg);

/***************************************************************************************************
 * @fn      halUart_GetRxAvailable()
 *
 * @brief   get available count of rx buffer
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
halUartBufLen_n halUart_GetRxAvailable( halUartCfg_t *a_ptUartCfg);

/***************************************************************************************************
 * @fn      halUart_Ch1RxIsr()
 *
 * @brief   uart channel 1 rx timeout
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_Ch1RxIsr( void );

/***************************************************************************************************
 * @fn      halUart_Ch1TxIsr()
 *
 * @brief   uart channel 1 tx timeout
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_Ch1TxIsr( void );


#endif /* __HAL_UART_H__ */
 
/***************************************************************************************************
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/