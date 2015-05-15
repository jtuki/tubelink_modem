#include "util_uart.h"
#include "stm8l15x_gpio.h"


#include <string.h>

/*!what should this file do?
 *receive data form uart, and store in buffer
 *send data(received from lora if) by uart
 */

/*!define
 *
 */
#define UART2_RX_MAX                        (256)
#define UART2_TX_MAX                        (512)
#define UART2_RX_ABOUT_FULL                 (UART2_RX_MAX >> 1)

#define UART_MSECS_TO_TICKS(n)              (n)
#define UART2_RX_TIMEOUT                    UART_MSECS_TO_TICKS(6)
/*!typedef
 */



/*!uart call back function
 */

typedef struct
{
    uint8_t u8RxBuf[UART2_RX_MAX];          /* receive buffer */
    UART_RX_CNT_TYPE_n nRxHead;             /* head position of rx buffer */
    UART_RX_CNT_TYPE_n nRxTail;             /* tail position of rx buffer */
    UART_RX_CNT_TYPE_n nRxMax;              /* max size of rx buffer */
    uint8_t u8RxTick;                       /* receive tick, will be set a const value when receive a new dat */
    uint8_t u8RxShdw;                       /* system tick shadow */
    
    uint8_t u8TxBuf[UART2_TX_MAX];          /* transmit buffer */
    UART_TX_CNT_TYPE_n nTxHead;             /* head positon of tx buffer */
    UART_TX_CNT_TYPE_n nTxTail;             /* tail position of tx buffer */
    UART_TX_CNT_TYPE_n nTxMax;              /* max size of tx buffer */
    uint8_t u8TxIntEn;                      /* tx int enable of not  */
    
    UART_CB_t pfUartCB;                     /* uart call back function */

}UART_INFO_t;



#if (USED_UART2 == 1)
static UART_INFO_t gs_tUart2Info;
#endif




static void util_uart2_init__(UART_INIT_t *a_ptUartInit, UART_CB_t a_pfUartPoll);




/*!util_uart_init
 */
void util_uart_init(UART_INIT_t *a_ptUartInit, UART_CB_t a_pfUartPoll)
{
    
    switch(a_ptUartInit->u8Uart)
    {
        case 0:
            break;
        case 1:
            break;
        case 2:
              util_uart2_init__(a_ptUartInit, a_pfUartPoll);
              break;
        default:
            break;
    }
     
}


/*!util_uart_uninit*/
void util_uart_uninit(void)
{
    USART_DeInit(USART1);
    GPIO_Init(GPIOC, GPIO_Pin_3, GPIO_Mode_Out_PP_High_Slow);   /* Uart 1 Tx */
    GPIO_Init(GPIOC, GPIO_Pin_2, GPIO_Mode_Out_PP_High_Slow);   /* Uart 1 Rx */
}


/*!util_uart_poll */
void util_uart_poll(uint8_t a_u8Uart)
{
    uint8_t u8ElapseTick = 0;
    uint8_t u8Tick = 0;
    UART_RX_CNT_TYPE_n nRxAvaCount = 0;
    UART_TX_CNT_TYPE_n nTxAvaCount = 0;
    uint8_t u8Evt = 0;
    UART_INFO_t *ptUartInfo = NULL;
    switch(a_u8Uart)
    {
        case UTIL_UART_CHN0:
#if (USED_UART0 == 1)
            ptUartInfo = &gs_tUart0Info;
#endif           
            break;
        case UTIL_UART_CHN1:
#if (USED_UART1 == 1)
            ptUartInfo = &gs_tUart1Info;
#endif
            break;
        case UTIL_UART_CHN2:
#if (USED_UART2 == 1)
            ptUartInfo = &gs_tUart2Info;
#endif
            break;
           
    }
    


    
    if((NULL != ptUartInfo) && (NULL != ptUartInfo->pfUartCB))  /* validate callback function */
    {
        u8Tick = UTIL_UART_GET_TICK() & 0xFF;       /* get system tick, only use low 8 bits */
        
        /* disable interrupt */
        __disable_interrupt();
        /* calculate elapse time tick */
        if(u8Tick >= ptUartInfo->u8RxShdw)
        {
            u8ElapseTick = u8Tick - ptUartInfo->u8RxShdw;
        }
        else
        {
            u8ElapseTick = 0xFF - ptUartInfo->u8RxShdw + u8Tick;
        }
        ptUartInfo->u8RxShdw = u8Tick;
        
        if(ptUartInfo->u8RxTick > u8ElapseTick)
        {
            ptUartInfo->u8RxTick -= u8ElapseTick;
        }
        else
        {
            ptUartInfo->u8RxTick = 0;
        }
        /* enable interrupt */
        __enable_interrupt();
        
        nRxAvaCount = util_uart_get_rx_available_count(UTIL_UART_CHN2); /* get available receive count */
        nTxAvaCount = util_uart_get_tx_available_count(UTIL_UART_CHN2); 
        if(nRxAvaCount >= (ptUartInfo->nRxMax - 1))
        {
            u8Evt = UTIL_UART_EVT_RX_FULL;
        }
        else if(nRxAvaCount >= (ptUartInfo->nRxMax >> 1))
        {
            u8Evt = UTIL_UART_EVT_RX_ABOUT_FULL;
        }
        else if(nRxAvaCount && 0 == ptUartInfo->u8RxTick)
        {
            u8Evt = UTIL_UART_EVT_RX_TIMEOUT;
        }
        else if(0 == gs_tUart2Info.u8TxIntEn)
        {
            u8Evt = UTIL_UART_EVT_TX_EMPTY;
        }
        else
        {
            u8Evt = UTIL_UART_EVT_IDLE;
        }
        
        /* @todo */
       // if(1 == ptUartInfo->u8TxIntEn)
        {
            //ptUartInfo->u8TxIntEn = 0;
           // u8Evt |= UTIL_UART_EVT_TX_EMPTY;
        }
        
        if(0 != u8Evt)
        {
            ptUartInfo->pfUartCB(u8Evt);
        }
    }


}


/*!uart_buffer_read
*/
uint16_t util_uart_read(uint8_t a_u8Uart, uint8_t *a_pu8Buf, uint16_t a_u16Len)
{
    UART_RX_CNT_TYPE_n nRetCnt = 0;
    UART_INFO_t *ptUartInfo = NULL;
    
    assert_param(NULL != a_pu8Buf);
    
    switch(a_u8Uart)
    {
        case UTIL_UART_CHN0:
#if(1 == USED_UART0)
            ptUartInfo = &gs_tUart0Info;
#endif
            break;
        case UTIL_UART_CHN1:
#if(1 == USED_UART1)
            ptUartInfo = &gs_tUart1Info;
#endif
            break;
        case UTIL_UART_CHN2:
#if(1 == USED_UART2)
            ptUartInfo = &gs_tUart2Info;
#endif
            break;
        default:
            break;
    }
    
    if(NULL != ptUartInfo)
    {
        while((nRetCnt < a_u16Len) && (ptUartInfo->nRxTail != ptUartInfo->nRxHead))
        {
            *a_pu8Buf = ptUartInfo->u8RxBuf[ptUartInfo->nRxHead];
            a_pu8Buf++;
            ptUartInfo->nRxHead++;
            
            if(ptUartInfo->nRxHead >= ptUartInfo->nRxMax)
            {
                ptUartInfo->nRxHead = 0;
            }
            nRetCnt ++;
        }
    }
    return nRetCnt;
}

/*!uart buffer write*/
uint16_t util_uart_write(uint8_t a_u8Uart, uint8_t *a_pu8Buf, uint16_t a_u16Len)
{
    UART_RX_CNT_TYPE_n nRetCount = 0;
    UART_INFO_t *ptUartInfo = NULL;
    uint8_t u8Data = 0;
    
    assert_param(NULL != a_pu8Buf);
    
    
    switch(a_u8Uart)
    {
        case UTIL_UART_CHN0:
#if(1 == USED_UART0)
            ptUartInfo = &gs_tUart0Info;
#endif
            break;
        case UTIL_UART_CHN1:
#if(1 == USED_UART1)
            ptUartInfo = &gs_tUart1Info;
#endif
            break;
        case UTIL_UART_CHN2:
#if(1 == USED_UART2)
            ptUartInfo = &gs_tUart2Info;
#endif
            break;
        default:
            break;
    }
    
    if(util_uart_get_tx_available_count(a_u8Uart) < a_u16Len)
    {
        nRetCount = 0;
    }
    else
    {
        for(nRetCount = 0; nRetCount < a_u16Len; nRetCount ++)
        {
            ptUartInfo->u8TxBuf[ptUartInfo->nTxTail] = *a_pu8Buf++;
            /*@todo txMT*/
            if(ptUartInfo->nTxTail >= (ptUartInfo->nTxMax - 1))
            {
                ptUartInfo->nTxTail = 0;
            }
            else
            {
                ptUartInfo->nTxTail++;
            }
        }
    }
    
    /* tx buffer is empty before, should send manual to trigger transmit complete interrupt */
    if(((0 == gs_tUart2Info.u8TxIntEn) || (TRUE == USART_GetFlagStatus(USART1, USART_IT_TXE)))\
        && (gs_tUart2Info.nTxTail != gs_tUart2Info.nTxHead))
    {
        gs_tUart2Info.u8TxIntEn = 1;
        /* enable tx interrupt*/
        //USART_ITConfig(USART1, USART_IT_TC, ENABLE);
        u8Data = gs_tUart2Info.u8TxBuf[gs_tUart2Info.nTxHead++];
        if(gs_tUart2Info.nTxHead >= gs_tUart2Info.nTxMax)
        {
            gs_tUart2Info.nTxHead = 0;
        }
        USART_SendData8(USART1, u8Data);
        
        
    }
    
    return nRetCount;
}


/*!util_uart2_init__
 */
void util_uart2_init__(UART_INIT_t *a_ptUartInit, UART_CB_t a_pfUartPoll)
{
#if (USED_UART2 == 1)
    USART_StopBits_TypeDef tStopBits = USART_StopBits_1;
    USART_Parity_TypeDef tParity = USART_Parity_No;
    GPIO_Init(GPIOC, GPIO_Pin_3, GPIO_Mode_Out_PP_High_Fast);
    GPIO_Init(GPIOC, GPIO_Pin_2, GPIO_Mode_In_FL_No_IT);
    assert_param(0 != a_ptUartInit);
    
    /* init gs_tUart2Info struct */
    memset(&gs_tUart2Info, 0, sizeof(gs_tUart2Info));
    gs_tUart2Info.pfUartCB = a_pfUartPoll;
    gs_tUart2Info.nRxMax = UART2_RX_MAX;
    gs_tUart2Info.nTxMax = UART2_TX_MAX;
    
    switch(a_ptUartInit->eStopBits)
    {
        case UART_STOP_BITS_10:
            tStopBits = USART_StopBits_1;
            break;
        case UART_STOP_BITS_15:
            tStopBits = USART_StopBits_1_5;
            break;
        case UART_STOP_BITS_20:
            tStopBits = USART_StopBits_2;
            break;
        default:
            tStopBits = USART_StopBits_1;
            break;
    }
    
    switch(a_ptUartInit->eParity)
    {
        case UART_PARITY_NONE:
            tParity = USART_Parity_No;
            break;
        case UART_PARITY_EVEN:
            tParity = USART_Parity_Even;
            break;
        case UART_PARITY_ODD:
            tParity = USART_Parity_Odd;
            break;
    }
    CLK_PeripheralClockConfig(CLK_Peripheral_USART1, ENABLE);         /* enable usart1 */
    /* uart init */
    USART_Init(USART1,
             a_ptUartInit->u32Baud,   /* baudrate setting */ 
             USART_WordLength_8b,       /* 8 bit data */
             tStopBits,                 /* stop bit */
             tParity,                   /* parity */
             USART_Mode_Rx|USART_Mode_Tx);       /* enable tx and rx */
    
    USART_ClearITPendingBit(USART1,USART_IT_TC);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_ITConfig(USART1, USART_IT_TC, ENABLE);
    ITC_SetSoftwarePriority(USART1_TX_IRQn, ITC_PriorityLevel_3);
    ITC_SetSoftwarePriority(USART1_RX_IRQn, ITC_PriorityLevel_3);
    USART_Cmd(USART1, ENABLE);
#endif
}


/*!util_uart_get_rx_available_count*/
UART_RX_CNT_TYPE_n util_uart_get_rx_available_count(uint8_t a_u8Uart)
{
    UART_RX_CNT_TYPE_n nRetCount = 0;
    UART_INFO_t *ptUartInfo = NULL;
 
    switch(a_u8Uart)
    {
        case UTIL_UART_CHN0:
#if(1 == USED_UART0)
            ptUartInfo = &gs_tUart0Info;
#endif
            break;
        case UTIL_UART_CHN1:
#if(1 == USED_UART1)
            ptUartInfo = &gs_tUart1Info;
#endif
            break;
        case UTIL_UART_CHN2:
#if(1 == USED_UART2)
            ptUartInfo = &gs_tUart2Info;
#endif
            break;
        default:
            break;
    }
    
    if(NULL != ptUartInfo)
    {
        nRetCount = (ptUartInfo->nRxTail >= ptUartInfo->nRxHead) ? \
            (ptUartInfo->nRxTail - ptUartInfo->nRxHead) : \
            (ptUartInfo->nRxMax - ptUartInfo->nRxHead + ptUartInfo->nRxTail);
    }
    
    return nRetCount;
}

/*!util_uart_get_rx_available_count*/
UART_RX_CNT_TYPE_n util_uart_get_tx_available_count(uint8_t a_u8Uart)
{
    UART_RX_CNT_TYPE_n nRetCount = 0;
    UART_INFO_t *ptUartInfo = NULL;
 
    switch(a_u8Uart)
    {
        case UTIL_UART_CHN0:
#if(1 == USED_UART0)
            ptUartInfo = &gs_tUart0Info;
#endif
            break;
        case UTIL_UART_CHN1:
#if(1 == USED_UART1)
            ptUartInfo = &gs_tUart1Info;
#endif
            break;
        case UTIL_UART_CHN2:
#if(1 == USED_UART2)
            ptUartInfo = &gs_tUart2Info;
#endif
            break;
        default:
            break;
    }
    
    if(NULL != ptUartInfo)
    {

        nRetCount = (ptUartInfo->nTxHead > ptUartInfo->nTxTail) ? \
            (ptUartInfo->nTxHead - ptUartInfo->nTxTail - 1) : \
            (ptUartInfo->nTxMax - ptUartInfo->nTxTail + ptUartInfo->nTxHead - 1);
    }
    
    return nRetCount;
}


uint8_t util_uart2_check_tx_empty(void)
{
    return gs_tUart2Info.nTxTail == gs_tUart2Info.nTxHead ? 1 : 0;
}


/*!util_uart_tx_isr_cb*/
void util_uart2_tx_isr_cb(void)
{
#if (1 == USED_UART2)
    if(RESET != USART_GetITStatus(USART1, USART_IT_TC))
    {
        USART_ClearITPendingBit(USART1,USART_IT_TC);
        if(gs_tUart2Info.nTxTail == gs_tUart2Info.nTxHead)
        {
            //USART_ITConfig(USART1, USART_IT_TC, DISABLE);           /* disable uart interrupt */
            gs_tUart2Info.u8TxIntEn = 0;
        }
        else
        {
            USART_SendData8(USART1, gs_tUart2Info.u8TxBuf[gs_tUart2Info.nTxHead++]);
            if(gs_tUart2Info.nTxHead >= gs_tUart2Info.nTxMax)
            {
                gs_tUart2Info.nTxHead = 0;
            }
            
        }
    }

#endif
}

/*!util_uart_rx_isr_cb */
void util_uart2_rx_isr_cb(void)
{
#if (1 == USED_UART2)
    if(RESET != USART_GetITStatus(USART1, USART_IT_RXNE))
    {
        USART_ClearITPendingBit(USART1,USART_IT_RXNE);
        gs_tUart2Info.u8RxBuf[gs_tUart2Info.nRxTail] = USART_ReceiveData8(USART1);
        
        if(gs_tUart2Info.nRxTail == gs_tUart2Info.nRxHead)
        {
            gs_tUart2Info.u8RxShdw = UTIL_UART_GET_TICK() & 0xFF;   /* re sync the shadow on any 1st byte received*/
        }
        
        if((++gs_tUart2Info.nRxTail) >= gs_tUart2Info.nRxMax)
        {
            gs_tUart2Info.nRxTail = 0;
        }
        gs_tUart2Info.u8RxTick = UART2_RX_TIMEOUT;
    }

#endif
}



