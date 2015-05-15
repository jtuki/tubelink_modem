/***************************************************************************************************
* @file name	hal_uart.c
* @data   		2015/03/29
* @auther   	chuanpengl
* @module   	hal uart module
* @brief 		hal_uart
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "hal_board_cfg.h"

#if (1 == _SW_UART)

#include "hal_uart.h"
#include "stm8l15x.h"
#include "systick\hal_systick.h"
/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */


/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
#if (1 == HAL_UART_1_EN)
static halUartCfg_t *gs_ptUart1Cfg;
#endif
#if (1 == HAL_UART_2_EN)
static halUartCfg_t *gs_ptUart2Cfg;
#endif
/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
#define HALUART_CAL_PORT(a_ptUartCfg)               USART1
#define HALUART_CAL_CLK_PERIPHERAL(a_ptUartCfg)     CLK_Peripheral_USART1
#define HALUART_CAL_TXIRQ(a_ptUartCfg)              USART1_TX_IRQn
#define HALUART_CAL_RXIRQ(a_ptUartCfg)              USART1_RX_IRQn

/* uart port control */
#define HALUART_INIT_PORT(a_ptUartCfg)              \
        GPIO_Init(GPIOC, GPIO_Pin_3, GPIO_Mode_Out_PP_High_Fast);\
        GPIO_Init(GPIOC, GPIO_Pin_2, GPIO_Mode_In_FL_No_IT);
#define HALUART_DEINIT_PORT(a_ptUartCfg)            \
        GPIO_Init(GPIOC, GPIO_Pin_3, GPIO_Mode_Out_PP_High_Slow);\
        GPIO_Init(GPIOC, GPIO_Pin_2, GPIO_Mode_Out_PP_High_Slow);
/* uart port control end */

#define HALUART_CAL_STOPBITS(a_ptUartCfg)   USART_StopBits_1
#define HALUART_CAL_PARITY(a_ptUartCfg)     USART_Parity_No
#define HALUART_CAL_WORDLENGTH(a_ptUartCfg) USART_WordLength_8b
#define HALUART_CAL_BAUDRATE(a_ptUartCfg)   \
        (a_ptUartCfg->baudRate == HAL_UART_BR_9600 ? 9600 : \
            (a_ptUartCfg->baudRate == HAL_UART_BR_19200 ? 115200 : \
                (a_ptUartCfg->baudRate == HAL_UART_BR_38400 ? 115200 : \
                    (a_ptUartCfg->baudRate == HAL_UART_BR_57600 ? 115200 : \
                        (a_ptUartCfg->baudRate == HAL_UART_BR_115200 ? 115200 : 9600\
                        )\
                    )\
                )\
            )\
        )
        
#define HALUART_CHK_TCI_ENABLE()   ((USART1->CR2 & 0x40) != 0 ? halUartTrue : halUartFalse)

#define HALUART_GET_TICK()      (systick_Get() & 0xFF)
#define HALUART_RX_TIME_ELAPSED(nStartTime)     \
            (nStartTime <= HALUART_GET_TICK() ? HALUART_GET_TICK() - nStartTime : \
                (0xFF - nStartTime + HALUART_GET_TICK() + 1))

/***************************************************************************************************
 * @fn      halUart_SaveCfgHandle__()
 *
 * @brief   save uart configure handle
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void halUart_SaveCfgHandle__( halUartCfg_t *a_ptUartCfg);

/***************************************************************************************************
 *  EXTERNAL FUNCTIONS IMPLEMENTATION
 */
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
void halUart_InitCfg( halUartCfg_t *a_ptUartCfg )
{
    halUartAssert(halUartNull != a_ptUartCfg);
    
    a_ptUartCfg->configured = halUartFalse;
} /* halUart_Init() */


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
void halUart_Init( halUartCfg_t *a_ptUartCfg, halUartCallBack_t a_hUartCB )
{
    halUartAssert(halUartNull != a_ptUartCfg);
    halUartAssert(halUartNull != a_hUartCB);
    
    halUart_SaveCfgHandle__(a_ptUartCfg);
    
    a_ptUartCfg->callBackFunc = a_hUartCB;
    
    /* init port */
    HALUART_INIT_PORT(a_ptUartCfg);


    CLK_PeripheralClockConfig(HALUART_CAL_CLK_PERIPHERAL(a_ptUartCfg), ENABLE);         /* enable usart1 */
    /* uart init */
    USART_Init(HALUART_CAL_PORT(a_ptUartCfg),
             HALUART_CAL_BAUDRATE(a_ptUartCfg),         /* baudrate setting */ 
             HALUART_CAL_WORDLENGTH(a_ptUartCfg),       /* 8 bit data */
             HALUART_CAL_STOPBITS(a_ptUartCfg),         /* stop bit */
             HALUART_CAL_PARITY(a_ptUartCfg),           /* parity */
             USART_Mode_Rx|USART_Mode_Tx);              /* enable tx and rx */
    
    USART_ClearITPendingBit(HALUART_CAL_PORT(a_ptUartCfg),USART_IT_TC);
    USART_ITConfig(HALUART_CAL_PORT(a_ptUartCfg), USART_IT_RXNE, ENABLE);
    USART_ITConfig(HALUART_CAL_PORT(a_ptUartCfg), USART_IT_TC, ENABLE);
    ITC_SetSoftwarePriority(HALUART_CAL_TXIRQ(a_ptUartCfg), ITC_PriorityLevel_3);
    ITC_SetSoftwarePriority(HALUART_CAL_RXIRQ(a_ptUartCfg), ITC_PriorityLevel_3);
    USART_Cmd(HALUART_CAL_PORT(a_ptUartCfg), ENABLE);
    
    a_ptUartCfg->configured = halUartTrue;
} /* halUart_Init() */


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
void halUart_DeInit( halUartCfg_t *a_ptUartCfg )
{
    halUartAssert(halUartNull != a_ptUartCfg);
    
    USART_DeInit(HALUART_CAL_PORT(a_ptUartCfg));
    HALUART_DEINIT_PORT(a_ptUartCfg);
}   /* halUart_DeInit() */


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
void halUart_Sleep( halUartCfg_t *a_ptUartCfg )
{
    
}   /* halUart_Sleep() */


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
void halUart_Wakeup( halUartCfg_t *a_ptUartCfg )
{
    
}   /* halUart_Wakeup() */


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
void halUart_Poll( halUartCfg_t *a_ptUartCfg )
{
    halUartBufLen_n nCnt = 0;
    halUartUint8 u8Evt = 0;
    halUartBool bRxTimeout = halUartFalse;
    
    halUartAssert(halUartNull != a_ptUartCfg);
    halUartAssert(halUartNull != a_ptUartCfg->callBackFunc);
    
    nCnt = halUart_GetRxAvailable(a_ptUartCfg);
    
    /* todo: calculate timeout */
    if(a_ptUartCfg->idleTimeout <= HALUART_RX_TIME_ELAPSED(a_ptUartCfg->rxChRvdTime))
    {
        bRxTimeout = halUartTrue;
    }
    
    if( nCnt >= (a_ptUartCfg->rx.nBufSize - 1) )
    {
        u8Evt = HAL_UART_RX_FULL;
    }
    else if( nCnt >= (a_ptUartCfg->rx.nBufSize >> 1) )
    {
        u8Evt = HAL_UART_RX_ABOUT_FULL;
    }
    else if( (nCnt > 0) && (halUartTrue == bRxTimeout) )
    {
        u8Evt = HAL_UART_RX_TIMEOUT;
    }
    
    /* todo: tx empty event */
    
    a_ptUartCfg->callBackFunc( 0, u8Evt );  /* parameter 1 is no use */
    
}   /* halUart_Poll() */


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
halUartBufLen_n halUart_Write( halUartCfg_t *a_ptUartCfg, halUartChar *a_pcBuf, halUartBufLen_n a_nLen)
{
    halUartBufLen_n nCnt = 0;
    
    halUartAssert(halUartNull != a_ptUartCfg);
    halUartAssert(halUartNull != a_pcBuf);

    if( a_nLen <= halUart_GetTxAvailable(a_ptUartCfg) )
    {
        for( nCnt = 0 ; nCnt < a_nLen; nCnt ++ )
        {
            a_ptUartCfg->tx.pBuffer[a_ptUartCfg->tx.nBufTail] = *a_pcBuf ++;
            a_ptUartCfg->tx.nBufTail = a_ptUartCfg->tx.nBufTail >= a_ptUartCfg->tx.nBufSize - 1 ? 0 : \
                a_ptUartCfg->tx.nBufTail + 1;
        }
        
        /* check tc interrupt */
        /* enter critical */
        ENTER_CRITICAL_SECTION();
        
        if( halUartFalse == HALUART_CHK_TCI_ENABLE() )
        {
            USART_ITConfig(HALUART_CAL_PORT(a_ptUartCfg), USART_IT_TC, ENABLE);
            USART_ClearITPendingBit(USART1,USART_IT_TC);
            USART_SendData8(HALUART_CAL_PORT(a_ptUartCfg), gs_ptUart1Cfg->tx.pBuffer[gs_ptUart1Cfg->tx.nBufHead++]);
            if(gs_ptUart1Cfg->tx.nBufHead >= gs_ptUart1Cfg->tx.nBufSize)
            {
                gs_ptUart1Cfg->tx.nBufHead = 0;
            }
        }
        
        /* exit critical */
        EXIT_CRITICAL_SECTION();
         
    }
    else
    {
        nCnt = 0;
    }
    
    return nCnt;
}


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
halUartBufLen_n halUart_Read( halUartCfg_t *a_ptUartCfg, halUartChar *a_pcBuf, halUartBufLen_n a_nLen)
{
    halUartBufLen_n nCnt = 0;
    
    halUartAssert(halUartNull != a_ptUartCfg);
    halUartAssert(halUartNull != a_pcBuf);
    
    while( (a_ptUartCfg->rx.nBufHead != a_ptUartCfg->rx.nBufTail) && ( nCnt < a_nLen ) )
    {
        *a_pcBuf ++ = a_ptUartCfg->rx.pBuffer[a_ptUartCfg->rx.nBufHead++];
        
        if(a_ptUartCfg->rx.nBufHead >= a_ptUartCfg->rx.nBufSize)
        {
            a_ptUartCfg->rx.nBufHead = 0;
        }
        
        nCnt ++;
    }
    
    return nCnt;
}   /* halUart_Read() */


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
halUartBufLen_n halUart_GetTxAvailable( halUartCfg_t *a_ptUartCfg)
{
    halUartBufLen_n nTemp = 0;
    halUartAssert(halUartNull != a_ptUartCfg);
    
    nTemp = a_ptUartCfg->tx.nBufHead;
    nTemp = (nTemp > a_ptUartCfg->tx.nBufTail) ? \
        (a_ptUartCfg->tx.nBufHead - a_ptUartCfg->tx.nBufTail - 1) : \
            (a_ptUartCfg->tx.nBufSize - a_ptUartCfg->tx.nBufTail + a_ptUartCfg->tx.nBufHead - 1);
    
    return nTemp;
}   /* halUart_GetTxAvailable() */

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
halUartBufLen_n halUart_GetRxAvailable( halUartCfg_t *a_ptUartCfg)
{
    halUartBufLen_n nTemp = 0;
    halUartAssert(halUartNull != a_ptUartCfg);
    
    nTemp = a_ptUartCfg->rx.nBufTail;
    nTemp = (nTemp > a_ptUartCfg->rx.nBufHead) ?  (nTemp - a_ptUartCfg->rx.nBufHead) : \
            (a_ptUartCfg->rx.nBufSize - a_ptUartCfg->rx.nBufHead + nTemp);
    
    return nTemp;
}   /* halUart_GetRxAvailable() */


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
void halUart_Ch1RxIsr( void )
{
    if(RESET != USART_GetITStatus(USART1, USART_IT_RXNE))
    {
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);
        gs_ptUart1Cfg->rx.pBuffer[gs_ptUart1Cfg->rx.nBufTail] = USART_ReceiveData8(USART1);
        
        gs_ptUart1Cfg->rxChRvdTime = HALUART_GET_TICK();
        
        if((++gs_ptUart1Cfg->rx.nBufTail) >= gs_ptUart1Cfg->rx.nBufSize)
        {
            gs_ptUart1Cfg->rx.nBufTail = 0;
        }

    }
}   /* halUart_Ch1RxIsr() */

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
void halUart_Ch1TxIsr( void )
{
    if(RESET != USART_GetITStatus(USART1, USART_IT_TC))
    {
        USART_ClearITPendingBit(USART1,USART_IT_TC);
        if(gs_ptUart1Cfg->tx.nBufTail == gs_ptUart1Cfg->tx.nBufHead)
        {
            USART_ITConfig(USART1, USART_IT_TC, DISABLE);           /* disable uart interrupt */
            gs_ptUart1Cfg->intEnable = halUartFalse;
        }
        else
        {
            USART_SendData8(USART1, gs_ptUart1Cfg->tx.pBuffer[gs_ptUart1Cfg->tx.nBufHead++]);
            if(gs_ptUart1Cfg->tx.nBufHead >= gs_ptUart1Cfg->tx.nBufSize)
            {
                gs_ptUart1Cfg->tx.nBufHead = 0;
            }
            
        }
    }
}   /* halUart_Ch1TxIsr() */

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */




/***************************************************************************************************
 * @fn      halUart_SaveCfgHandle__()
 *
 * @brief   save uart configure handle
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void halUart_SaveCfgHandle__( halUartCfg_t *a_ptUartCfg)
{
#if (1 == HAL_UART_1_EN)
    if(a_ptUartCfg->port == HAL_UART_PORT_0)
    {
        gs_ptUart1Cfg = a_ptUartCfg;
    }
#endif
#if (1 == HAL_UART_2_EN )
    if(a_ptUartCfg->port == HAL_UART_PORT_0)
    {
        gs_ptUart2Cfg = a_ptUartCfg;
    }
#endif
}
#endif
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/