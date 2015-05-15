#ifndef __UTIL_UART_H__
#define __UTIL_UART_H__
#include "stm8l15x.h"
#include "systick\hal_systick.h"


/* !select used device 
 * 1 - used
 * 0 - not used
 */
#define USED_UART2          (1)
     
     
    
#define UTIL_UART_CHN0      ((uint8_t)0)
#define UTIL_UART_CHN1      ((uint8_t)1)
#define UTIL_UART_CHN2      ((uint8_t)2)

/* @todo-should modify to get tick function
 *get system tick 
 */
extern uint32_t time_tick_get(void);
#define UTIL_UART_GET_TICK()      systick_Get()
     
     
     
/* UART Events */
#define UTIL_UART_EVT_IDLE              (0x00)
#define UTIL_UART_EVT_RX_FULL           (0x01)
#define UTIL_UART_EVT_RX_ABOUT_FULL     (0x02)
#define UTIL_UART_EVT_RX_TIMEOUT        (0x04)
#define UTIL_UART_EVT_TX_FULL           (0x08)
#define UTIL_UART_EVT_TX_EMPTY          (0x10)



#define UART2_RX_MAX                        (256)
#define UART2_TX_MAX                        (512)
#define UART2_RX_ABOUT_FULL                 (UART2_RX_MAX >> 1)

#define UART_MSECS_TO_TICKS(n)              (n)
#define UART2_RX_TIMEOUT                    UART_MSECS_TO_TICKS(6)
/*!RX_BUF_CNT_TYPE
 */
#if (UART2_RX_MAX < 256)
typedef uint8_t UART_RX_CNT_TYPE_n; 
#else
typedef uint16_t UART_RX_CNT_TYPE_n; 
#endif

/*!TX_BUF_CNT_TYPE
 */
#if (UART2_TX_MAX < 256)
typedef uint8_t UART_TX_CNT_TYPE_n; 
#else
typedef uint16_t UART_TX_CNT_TYPE_n; 
#endif

typedef enum
{
    UART_STOP_BITS_05 = 0,      /* half bit stop bits */
    UART_STOP_BITS_10 = 1,      /* one bit stop bits */
    UART_STOP_BITS_15 = 2,      /* one and half stop bits */
    UART_STOP_BITS_20 = 3       /* two stop bits */
}UART_STOP_BITS_e;

typedef enum
{
  UART_PARITY_NONE = 0,         /* no parity */
  UART_PARITY_EVEN = 1,         /* even parity */
  UART_PARITY_ODD = 2           /* odd parity */
}UART_PARITY_e;

typedef struct
{
    uint8_t u8Uart;
    UART_STOP_BITS_e eStopBits;
    UART_PARITY_e eParity;
    uint32_t u32Baud;
}UART_INIT_t;
typedef void (*UART_CB_t)(uint8_t a_u8Evt);

extern void util_uart_init(UART_INIT_t *a_ptUartInit, UART_CB_t a_pfUartPoll);
extern void util_uart_uninit(void);
extern void util_uart_poll(uint8_t a_u8Uart);
extern uint16_t util_uart_read(uint8_t a_u8Uart, uint8_t *a_pu8Buf, uint16_t a_u16Len);
extern uint16_t util_uart_write(uint8_t a_u8Uart, uint8_t *a_pu8Buf, uint16_t a_u16Len);
extern uint8_t util_uart2_check_tx_empty(void);
extern void util_uart2_tx_isr_cb(void);
extern void util_uart2_rx_isr_cb(void);
extern UART_RX_CNT_TYPE_n util_uart_get_rx_available_count(uint8_t a_u8Uart);
extern UART_RX_CNT_TYPE_n util_uart_get_tx_available_count(uint8_t a_u8Uart);
 
#endif
