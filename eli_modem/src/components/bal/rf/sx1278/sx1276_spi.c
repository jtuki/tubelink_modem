
#define USE_SX1276_RADIO
#ifdef USE_SX1276_RADIO

#include "sx1276_spi.h"

#define SX1276_SPI_HARDWARE



#if(PLATFORM == SX1276_DISCOVERY )
#include "stm8l15x_gpio.h"
#endif

/*!
 * spi sck I/O definitions
 */
#if(PLATFORM == SX1276_DISCOVERY )
#define SCK_PORT                        GPIOB
#define SCK_PIN                         GPIO_Pin_5
#else
#error "spi sck I/O not define!"
#endif

/*!
 * spi miso I/O definitions
 */
#if(PLATFORM == SX1276_DISCOVERY )
#define MISO_PORT                       GPIOB
#define MISO_PIN                        GPIO_Pin_7
#else
#error "spi miso I/O not define!"
#endif

/*!
 * spi mosi I/O definitions
 */
#if(PLATFORM == SX1276_DISCOVERY )
#define MOSI_PORT                       GPIOB
#define MOSI_PIN                        GPIO_Pin_6
#else
#error "spi mosi I/O not define!"
#endif


#ifndef SX1276_SPI_HARDWARE


#if 0
void sx1276_spi_delay(void)
{
  asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	asm("nop");	
}
#else
#define sx1276_spi_delay()
#endif


void sx1276_spi_init(void)
{
#if(PLATFORM == SX1276_DISCOVERY )
    GPIO_Init(SCK_PORT, SCK_PIN, GPIO_Mode_Out_PP_Low_Fast);        /* init sck as output fast */
    GPIO_Init(MOSI_PORT, MOSI_PIN, GPIO_Mode_Out_PP_High_Fast);     /* init mosi as output fast */
    GPIO_Init(MISO_PORT, MISO_PIN, GPIO_Mode_In_FL_No_IT);          /* init miso as input without interrupt */
#endif
}

    
void sx1276_spi_write(uint8_t a_u8Data)
{
#if(PLATFORM == SX1276_DISCOVERY )
    uint8_t u8BitCnt;
    
    GPIO_ResetBits(SCK_PORT, SCK_PIN);                               /* sck = 0 */

  
    for(u8BitCnt = 8; u8BitCnt != 0; u8BitCnt --)
    {
        GPIO_ResetBits(SCK_PORT, SCK_PIN);                           /* sck = 0 */
        sx1276_spi_delay();
        if(a_u8Data & 0x80)
        {
            GPIO_SetBits(MOSI_PORT, MOSI_PIN);                    /* mosi = 1 */
        }
        else
        {
            GPIO_ResetBits(MOSI_PORT, MOSI_PIN);                     /* mosi = 0 */
        }
        sx1276_spi_delay();
        GPIO_SetBits(SCK_PORT, SCK_PIN);                          /* sck = 1 */
        sx1276_spi_delay();
        a_u8Data <<= 1;
    }
    GPIO_ResetBits(SCK_PORT, SCK_PIN);                               /* sck = 0 */
#endif
}


uint8_t sx1276_spi_read(void)
{
#if(PLATFORM == SX1276_DISCOVERY )
    uint8_t u8ReadData = 0;
    uint8_t u8BitCnt = 0;
    
    /* read 8 bit data */
    for(u8BitCnt = 8; u8BitCnt != 0; u8BitCnt --)
    {
        u8ReadData <<= 1;
        GPIO_SetBits(SCK_PORT, SCK_PIN);                          /* sck = 1 */
        sx1276_spi_delay();
        /* read data from miso I/O */
        if(0 != GPIO_ReadInputDataBit(MISO_PORT, MISO_PIN))
        {
            u8ReadData |= 0x01;
        }
        else
        {
            u8ReadData &= 0xfe;
        }
        sx1276_spi_delay();
        
        GPIO_ResetBits(SCK_PORT, SCK_PIN);                           /* sck = 0 */
        sx1276_spi_delay();

    }
    //GPIO_WriteLow(SCK_PORT, SCK_PIN);                               /* sck = 0 */

    return(u8ReadData);
#endif
}


#else
void sx1276_spi_init(void)
{
    SPI_DeInit(SPI1);
    GPIO_Init(SCK_PORT, SCK_PIN, GPIO_Mode_Out_PP_Low_Fast);        /* init sck as output fast */
    GPIO_Init(MOSI_PORT, MOSI_PIN, GPIO_Mode_Out_PP_High_Fast);     /* init mosi as output fast */
    GPIO_Init(MISO_PORT, MISO_PIN, GPIO_Mode_In_FL_No_IT);          /* init miso as input without interrupt */
    CLK_PeripheralClockConfig(CLK_Peripheral_SPI1, ENABLE);         /* enable clock */
    SPI_Init(SPI1, SPI_FirstBit_MSB, SPI_BaudRatePrescaler_2, SPI_Mode_Master, SPI_CPOL_Low, SPI_CPHA_1Edge, SPI_Direction_2Lines_FullDuplex, SPI_NSS_Soft, SPI_CRCPR_RESET_VALUE);
    SPI_ClearFlag(SPI1, SPI_FLAG_RXNE);
    SPI_Cmd(SPI1, ENABLE);                                    /* enable spi */
}


void sx1276_spi_deinit(void)
{
    SPI_DeInit(SPI1);
    GPIO_Init(SCK_PORT, SCK_PIN, GPIO_Mode_Out_PP_High_Slow);   /* SCK_PORT */
    GPIO_Init(MOSI_PORT, MOSI_PIN, GPIO_Mode_Out_PP_High_Slow);   /* MOSI_PORT */
    GPIO_Init(MISO_PORT, MISO_PIN, GPIO_Mode_In_PU_No_IT);        /* MISO_PIN */
}




void sx1276_spi_write(uint8_t a_u8Data)
{
    while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);     /* wait for txe */
    SPI_SendData(SPI1, a_u8Data);                               /* send byte */
    while (SPI_GetFlagStatus(SPI1, SPI_FLAG_RXNE) == RESET);  /* 等待可读取 */

    SPI_ReceiveData(SPI1);
}


uint8_t sx1276_spi_read(void)
{
    
    while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);     /* wait for txe */
    //SPI_ClearFlag(SPI1, SPI_FLAG_RXNE);
    SPI_SendData(SPI1, 0xFF);                               /* send byte */
    
    while (SPI_GetFlagStatus(SPI1, SPI_FLAG_RXNE) == RESET);  /* 等待可读取 */

    return SPI_ReceiveData(SPI1);                                   /* 返回RX数据 */

}
#endif

/* end of USE_SX1276_RADIO */
#endif