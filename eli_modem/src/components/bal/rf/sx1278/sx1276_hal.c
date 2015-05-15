/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       sx1276-Hal.c
 * \brief      SX1276 Hardware Abstraction Layer
 *
 * \version    2.0.B2 
 * \date       Nov 21 2012
 * \author     Miguel Luis
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */

#define USE_SX1276_RADIO
#ifdef USE_SX1276_RADIO

#include "sx1276_hal.h"
#include "sx1276_spi.h"


#if(PLATFORM == SX1276_DISCOVERY )
#include "stm8l15x_gpio.h"
#endif

/*!
 * SX1276 RESET I/O definitions
 */
#if(PLATFORM == SX1276_DISCOVERY )
#define RESET_IOPORT                                GPIOA
#define RESET_PIN                                   GPIO_Pin_4
#else
#error "not define sx1276 reset i/o"
#endif

/*!
 * SX1276 SPI NSS I/O definitions
 */
#if(PLATFORM == SX1276_DISCOVERY )
#define NSS_IOPORT                                  GPIOB
#define NSS_PIN                                     GPIO_Pin_4
#else
#error "not define sx1276 spi nss i/o"
#endif

/*!
 * SX1276 DIO pins  I/O definitions
 */
#if(PLATFORM == SX1276_DISCOVERY )
#define DIO0_IOPORT                                 GPIOA
#define DIO0_PIN                                    GPIO_Pin_3
#else
#error "not define sx1276 dio 0 i/o"
#endif

#if(PLATFORM == SX1276_DISCOVERY )
#define DIO1_IOPORT                                 GPIOC
#define DIO1_PIN                                    GPIO_Pin_6
#else
#error "not define sx1276 dio 1 i/o"
#endif

#if(PLATFORM == SX1276_DISCOVERY )
#define DIO2_IOPORT                                 GPIOC
#define DIO2_PIN                                    GPIO_Pin_5
#else
#error "not define sx1276 dio 2 i/o"
#endif

#if(PLATFORM == SX1276_DISCOVERY )
#define DIO3_IOPORT                                 GPIOA
#define DIO3_PIN                                    GPIO_Pin_5
#else
#error "not define sx1276 dio 3 i/o"
#endif
     
#if(PLATFORM == SX1276_DISCOVERY )
#define DIO4_IOPORT                                 GPIOD
#define DIO4_PIN                                    GPIO_Pin_1
#else
#error "not define sx1276 dio 4 i/o"
#endif

#if(PLATFORM == SX1276_DISCOVERY )
#define DIO5_IOPORT                                 GPIOD
#define DIO5_PIN                                    GPIO_Pin_4
#else
#error "not define sx1276 dio 5 i/o"
#endif

#if(PLATFORM == SX1276_DISCOVERY )
#define RXTX_IOPORT                                 GPIOC
#define RXTX_PIN                                    GPIO_PIN_1
#else
#error "not define sx1276 dio RXTX i/o"
#endif

#if(PLATFORM == SX1276_DISCOVERY )
#define CTRL1_IOPORT                                 GPIOD
#define CTRL1_PIN                                    GPIO_Pin_0
#else
#error "not define sx1276 dio 0 i/o"
#endif

#if(PLATFORM == SX1276_DISCOVERY )
#define CTRL2_IOPORT                                 GPIOB
#define CTRL2_PIN                                    GPIO_Pin_3
#else
#error "not define sx1276 dio 0 i/o"
#endif

#if 1
static void sx1276_hal_delay(void);
void sx1276_hal_delay(void)
{
  asm("nop");asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	
}
#else
#define sx1276_hal_delay()
#endif



void SX1276InitIo(void)
{
#if(PLATFORM == SX1276_DISCOVERY)
    /* init sx1276 pins */  
    GPIO_Init(RESET_IOPORT, RESET_PIN, GPIO_Mode_Out_PP_High_Fast); /* init sx1276 reset I/O as output */
    GPIO_Init(NSS_IOPORT, NSS_PIN, GPIO_Mode_Out_PP_High_Fast);     /* init sx1276 nss I/O as output */
    GPIO_Init(DIO0_IOPORT, DIO0_PIN, GPIO_Mode_In_PU_No_IT);        /* init sx1276 dio 0 I/O as input without int */
    GPIO_Init(DIO1_IOPORT, DIO1_PIN, GPIO_Mode_In_PU_No_IT);        /* init sx1276 dio 1 I/O as input without int */
    GPIO_Init(DIO2_IOPORT, DIO2_PIN, GPIO_Mode_In_PU_No_IT);        /* init sx1276 dio 2 I/O as input without int */
    GPIO_Init(DIO3_IOPORT, DIO3_PIN, GPIO_Mode_In_PU_No_IT);        /* init sx1276 dio 3 I/O as input without int */
    GPIO_Init(DIO4_IOPORT, DIO4_PIN, GPIO_Mode_In_PU_No_IT);        /* init sx1276 dio 4 I/O as input without int */
    GPIO_Init(DIO5_IOPORT, DIO5_PIN, GPIO_Mode_In_PU_No_IT);        /* init sx1276 dio 5 I/O as input without int */
//    GPIO_Init(RXTX_IOPORT, RXTX_PIN, GPIO_Mode_In_FL_No_IT);        /* init sx1276 rx_tx I/O as input without int */
    GPIO_Init(CTRL1_IOPORT, CTRL1_PIN, GPIO_Mode_Out_PP_High_Fast); /* init sx1276 ctrl 1 I/O as output */
    GPIO_Init(CTRL2_IOPORT, CTRL2_PIN, GPIO_Mode_Out_PP_High_Fast); /* init sx1276 ctrl 2 I/O as output */   
    
    SX1276WriteRxTx(0);
#endif
}

void sx1276_deinit(void)
{
    GPIO_Init(RESET_IOPORT, RESET_PIN, GPIO_Mode_Out_PP_High_Slow/*GPIO_Mode_In_PU_No_IT*/);          /* init sx1276 reset I/O as input without int */
    GPIO_Init(NSS_IOPORT, NSS_PIN, GPIO_Mode_Out_PP_High_Slow);         /* init sx1276 nss I/O as output */
    GPIO_Init(DIO0_IOPORT, DIO0_PIN, GPIO_Mode_In_FL_No_IT/*GPIO_Mode_Out_PP_Low_Slow*/);        /* init sx1276 dio 0 I/O as output */
    GPIO_Init(DIO1_IOPORT, DIO1_PIN, GPIO_Mode_In_FL_No_IT/*GPIO_Mode_Out_PP_Low_Slow*/);        /* init sx1276 dio 1 I/O as output */
    GPIO_Init(DIO2_IOPORT, DIO2_PIN, GPIO_Mode_In_FL_No_IT/*GPIO_Mode_Out_PP_Low_Slow*/);        /* init sx1276 dio 2 I/O as output */
    GPIO_Init(DIO3_IOPORT, DIO3_PIN, GPIO_Mode_In_FL_No_IT/*GPIO_Mode_Out_PP_Low_Slow*/);        /* init sx1276 dio 3 I/O as output */
    GPIO_Init(DIO4_IOPORT, DIO4_PIN, GPIO_Mode_In_FL_No_IT/*GPIO_Mode_Out_PP_Low_Slow*/);        /* init sx1276 dio 4 I/O as output */
    GPIO_Init(DIO5_IOPORT, DIO5_PIN, GPIO_Mode_In_FL_No_IT/*GPIO_Mode_Out_PP_Low_Slow*/);        /* init sx1276 dio 5 I/O as output */
    GPIO_Init(CTRL2_IOPORT, CTRL2_PIN, GPIO_Mode_Out_PP_Low_Slow);      /* CTRL2_IOPORT */
    GPIO_Init(CTRL1_IOPORT, CTRL1_PIN, GPIO_Mode_Out_PP_Low_Slow);      /*CTRL1_IOPORT*/
}


void sx1276_EnDio2Int(uint8_t en)
{
    /* disable */
    if(0 == en)
    {
        GPIO_Init(DIO2_IOPORT, DIO2_PIN, GPIO_Mode_In_PU_No_IT); 
    }
    /* enable */
    else
    {
        
        __disable_interrupt();
        EXTI_SetPinSensitivity(EXTI_Pin_5, EXTI_Trigger_Rising);   /* dio2 pin exit interrupt */
        GPIO_Init(DIO2_IOPORT, DIO2_PIN, GPIO_Mode_In_PU_IT);        /* init sx1276 dio 2 I/O as input with int */
        ITC_SetSoftwarePriority(EXTI5_IRQn, ITC_PriorityLevel_1);
        __enable_interrupt();
    }
}




void SX1276SetReset( uint8_t state )
{
    if( state == RADIO_RESET_ON )
    {
#if(PLATFORM == SX1276_DISCOVERY)
        GPIO_ResetBits(RESET_IOPORT, RESET_PIN);                     /* reset pin = low level */
#endif
    }
    else
    {
#if(PLATFORM == SX1276_DISCOVERY)
        GPIO_SetBits(RESET_IOPORT, RESET_PIN);                    /* reset pin = high level */
#endif
    }
}

void SX1276Write( uint8_t addr, uint8_t data )
{
#if(PLATFORM == SX1276_DISCOVERY)
    GPIO_ResetBits(NSS_IOPORT, NSS_PIN);                             /* nss pin = low level */
    sx1276_hal_delay();
    
    sx1276_spi_write( addr | 0x80 );                                        /* fifo write access */
    
    /* write all data to sx1276 fifo through spi interface */
    sx1276_spi_write( data );
    
    sx1276_hal_delay();
    GPIO_SetBits(NSS_IOPORT, NSS_PIN);                            /* nss pin =  high level */
#endif
}

void SX1276Read( uint8_t addr, uint8_t *data )
{
#if(PLATFORM == SX1276_DISCOVERY)
    GPIO_ResetBits(NSS_IOPORT, NSS_PIN);                             /* nss pin = low level */
    sx1276_hal_delay();
    sx1276_spi_write( addr & 0x7F );                                        /* fifo read access */

    /* read all data from sx1276 fifo through spi interface */
    *data = sx1276_spi_read();

    sx1276_hal_delay();
    GPIO_SetBits(NSS_IOPORT, NSS_PIN);                            /* nss pin =  high level */
#endif
}

void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;
    
#if(PLATFORM == SX1276_DISCOVERY)
    GPIO_ResetBits(NSS_IOPORT, NSS_PIN);                             /* nss pin = low level */
    sx1276_hal_delay();
    
    sx1276_spi_write( addr | 0x80 );                                        /* fifo write access */
    
    /* write all data to sx1276 fifo through spi interface */
    for( i = 0; i < size; i++ )
    {
        sx1276_spi_write( buffer[i] );
    }
    
    sx1276_hal_delay();
    GPIO_SetBits(NSS_IOPORT, NSS_PIN);                            /* nss pin =  high level */
#endif


}

void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;
#if(PLATFORM == SX1276_DISCOVERY)
    GPIO_ResetBits(NSS_IOPORT, NSS_PIN);                             /* nss pin = low level */
    sx1276_hal_delay();
    sx1276_spi_write( addr & 0x7F );                                        /* fifo read access */

    /* read all data from sx1276 fifo through spi interface */
    for( i = 0; i < size; i++ )
    {
        buffer[i] = sx1276_spi_read();
    }
    sx1276_hal_delay();
    GPIO_SetBits(NSS_IOPORT, NSS_PIN);                            /* nss pin =  high level */
#endif
}

void SX1276WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1276WriteBuffer( 0, buffer, size );
}

void SX1276ReadFifo( uint8_t *buffer, uint8_t size )
{
#if(PLATFORM == SX1276_DISCOVERY)
    SX1276ReadBuffer( 0, buffer, size );
#endif
}

inline uint8_t SX1276ReadDio0( void )
{
#if(PLATFORM == SX1276_DISCOVERY)
  return GPIO_ReadInputDataBit(DIO0_IOPORT, DIO0_PIN) == 0 ? 0 : 1;
#endif
}

inline uint8_t SX1276ReadDio1( void )
{
#if(PLATFORM == SX1276_DISCOVERY)
    return GPIO_ReadInputDataBit( DIO1_IOPORT, DIO1_PIN ) == 0 ? 0 : 1;
#endif
}

inline uint8_t SX1276ReadDio2( void )
{
#if(PLATFORM == SX1276_DISCOVERY)
    return GPIO_ReadInputDataBit( DIO2_IOPORT, DIO2_PIN ) == 0 ? 0 : 1;
#endif
}

inline uint8_t SX1276ReadDio3( void )
{
#if(PLATFORM == SX1276_DISCOVERY)
    return GPIO_ReadInputDataBit( DIO3_IOPORT, DIO3_PIN ) == 0 ? 0 : 1;
#endif
}

inline uint8_t SX1276ReadDio4( void )
{
#if(PLATFORM == SX1276_DISCOVERY)
    return 0;                                                       /* no use in this application */
#endif
}

inline uint8_t SX1276ReadDio5( void )
{
#if(PLATFORM == SX1276_DISCOVERY)
    return GPIO_ReadInputDataBit( DIO5_IOPORT, DIO5_PIN ) == 0 ? 0 : 1;
#endif
}

inline uint8_t SX1276ReadRxTx( void )
{
#if(PLATFORM == SX1276_DISCOVERY)
    //return GPIO_ReadInputDataBit( RXTX_IOPORT, RXTX_PIN ) == 0 ? 0 : 1;
    return 0;
#endif
}


inline void SX1276WriteRxTx( uint8_t txEnable )
{
    /* do not understand this function's use */
    
#if(PLATFORM == SX1276_DISCOVERY)
    if(0 == txEnable)
    {
        GPIO_ResetBits(CTRL1_IOPORT, CTRL1_PIN);                     /* nss pin = low level */
        GPIO_SetBits(CTRL2_IOPORT, CTRL2_PIN);                    /* nss pin = high level */
    }
    else
    {
        GPIO_SetBits(CTRL1_IOPORT, CTRL1_PIN);                    /* nss pin = high level */
        GPIO_ResetBits(CTRL2_IOPORT, CTRL2_PIN);                     /* nss pin = low level */
    }
    
    //sx1276_hal_delay();
   // sx1276_hal_delay();
   // sx1276_hal_delay();
    //sx1276_hal_delay();
#endif
}

#endif // USE_SX1276_RADIO
