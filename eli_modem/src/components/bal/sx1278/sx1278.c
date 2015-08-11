/***************************************************************************************************
* @file name    rf_sx1278.c
* @data         2014/11/17
* @auther       chuanpengl
* @module       sx1278 hal
* @brief        sx1278 process
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "sx1278.h"

#if (BAL_USE_SX1278 == BAL_MODULE_ON)

#include "sx1278_reg.h"
#include "sx1278_dio.h"
#include "sx1278_spi.h"

#include <stdio.h>
#include <string.h>


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */
//#define _SX1278DEBUG
/***************************************************************************************************
 * MACROS
 */
#define XTAL_FREQ                                   32000000
#define FREQ_STEP                                   61.03515625

#define RSSI_OFFSET_LF                              -155.0
#define RSSI_OFFSET_HF                              -150.0

#define NOISE_ABSOLUTE_ZERO                         -174.0

#define NOISE_FIGURE_LF                                4.0
#define NOISE_FIGURE_HF                                6.0 

#define HOPPING_FREQUENCIES(IDX)    (HoppingFrequenciesDelta[IDX] + LoRaSettings.RFFrequency)
//#define HOPPING_FREQUENCIES(IDX)    (LoRaSettings.RFFrequency + HoppingFrequenciesDelta[5])
#define TICK_RATE_MS( ms )          ( ms )
/***************************************************************************************************
 * CONSTANTS
 */
/*!
 * Precomputed signal bandwidth log values
 * Used to compute the Packet RSSI value.
 */
const double SignalBwLog[] =
{
    3.8927900303521316335038277369285,  // 7.8 kHz
    4.0177301567005500940384239336392,  // 10.4 kHz
    4.193820026016112828717566631653,   // 15.6 kHz
    4.31875866931372901183597627752391, // 20.8 kHz
    4.4948500216800940239313055263775,  // 31.2 kHz
    4.6197891057238405255051280399961,  // 41.6 kHz
    4.795880017344075219145044421102,   // 62.5 kHz
    5.0969100130080564143587833158265,  // 125 kHz
    5.397940008672037609572522210551,   // 250 kHz
    5.6989700043360188047862611052755   // 500 kHz
};

const double RssiOffsetLF[] =
{   // These values need to be specify in the Lab
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
    -155.0,
};

/*!
 * Frequency hopping frequencies table
 */
static const rfInt32 HoppingFrequenciesDelta[] =
{
    50000>>1,
    100000>>1,
    150000>>1,
    200000>>1,
    250000>>1,
    300000>>1,
    350000>>1,
    400000>>1,
    450000>>1,
    500000>>1,
    550000>>1,
    600000>>1,
    650000>>1,
    700000>>1,
    750000>>1,
    800000>>1,
    850000>>1,
    900000>>1,
    950000>>1,
    900000>>1,
    850000>>1,
    800000>>1,
    750000>>1,
    700000>>1,
    650000>>1,
    600000>>1,
    550000>>1,
    500000>>1,
    450000>>1,
    400000>>1,
    350000>>1,
    300000>>1,
    250000>>1,
    200000>>1,
    150000>>1,
    100000>>1,
     50000>>1,
         0>>1,
    100000>>1,
    200000>>1,
    300000>>1,
    400000>>1,
    500000>>1,
    600000>>1,
    700000>>1,
    800000>>1,
    900000>>1,
     50000>>1,
    150000>>1,
    250000>>1,
    350000>>1,
};


/***************************************************************************************************
 * TYPEDEFS
 */

/* define irq flags */
typedef struct
{
    rfUint8 bitRxTimeout:1;
    rfUint8 bitRxDone:1;
    rfUint8 bitPayloadCrcErr:1;
    rfUint8 bitValid:1;
    rfUint8 bitTxDne:1;
    rfUint8 bitCadDone:1;
    rfUint8 bitFhssChangeChn:1;
    rfUint8 bitCadDetected:1;
}SX1278_IRQ_FLAGS_t;



/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */

/* SX1276 registers variable */
static rfUint8 SX1276Regs[0x70] = {0};

tSX1276LR* SX1276LR;

// Default settings
tLoRaSettings LoRaSettings =
{
    423000000,        // RFFrequency
    20,               // Power
    8,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    7,                // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    3,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    rfTrue,             // CrcOn [0: OFF, 1: ON]
    rfFalse,            // ImplicitHeaderOn [0: OFF, 1: ON]
    rfTrue,             // RxSingleOn [0: Continuous, 1 Single]
    rfFalse,            // FreqHopOn [0: OFF, 1: ON]
    4,                // HopPeriod Hops every frequency hopping period symbols
    2000,              // TxPacketTimeout
    /* here, if in cad mode, rx timeout value may be need greater then sleep time */
    20,              // RxPacketTimeout
    128,              // PayloadLength (used for implicit header mode)
    32,              /* preamble length */
};


//static SX1278_IRQ_FLAGS_t gs_tIrqFlags = {0};

static rfUint8 gs_u8PacketSize = 0;
static rfUint8 gs_au8PacketBuffer[RF_BUFFER_SIZE] = {0};

static rfInt8 RxPacketSnrEstimate;
static double RxPacketRssiValue;

static rfUint32 gs_u32PacketTxTime = 0;
static rfUint32 gs_u32RoutineTmo = 0;
/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */

/***************************************************************************************************
 * @fn      sx1278_Write__()
 *
 * @brief   sx1278 write data to addr
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - write address
 *          a_u8Data  - write data
 *
 * @return  none
 */
static void sx1278_Write__( rfUint8 a_u8Addr, rfUint8 a_u8Data );

/***************************************************************************************************
 * @fn      sx1278_Read__()
 *
 * @brief   sx1278 read data from address
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - read address
 *          a_pu8Data  - read data
 *
 * @return  none
 */
static void sx1278_Read__( rfUint8 a_u8Addr, rfUint8 *a_pu8Data );

/***************************************************************************************************
 * @fn      sx1278_WriteBuffer__()
 *
 * @brief   sx1278 write data to buffer
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - write address
 *          a_pu8Buffer  - write data
 *          a_u16Size  - write data size
 *
 * @return  none
 */
static void sx1278_WriteBuffer__( rfUint8 a_u8Addr, rfUint8 *a_pu8Buffer, rfUint16 a_u16Size );

/***************************************************************************************************
 * @fn      sx1278_ReadBuffer__()
 *
 * @brief   sx1278 read data from buffer
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - read address
 *          a_pu8Buffer  - read data
 *          a_u16Size  - read data size
 *
 * @return  none
 */
static void sx1278_ReadBuffer__( rfUint8 a_u8Addr, rfUint8 *a_pu8Buffer, rfUint16 a_u16Size );

/***************************************************************************************************
 * @fn      sx1278_WriteFifo__()
 *
 * @brief   sx1278 write data to fifo
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Buffer  - write data
 *          a_u16Size  - write data size
 *
 * @return  none
 */
static void sx1278_WriteFifo__( rfUint8 *a_pu8Buffer, rfUint16 a_u16Size );

/***************************************************************************************************
 * @fn      sx1278_ReadFifo__()
 *
 * @brief   sx1278 read data from fifo
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Buffer  - read data
 *          a_u16Size  - read data size
 *
 * @return  none
 */
static void sx1278_ReadFifo__( uint8_t *a_pu8Buffer, uint8_t a_u16Size );


/***************************************************************************************************
 * @fn      SX1276Reset()
 *
 * @brief   lora reset
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void SX1276Reset( void );


/***************************************************************************************************
 * @fn      SX1276SetLoRaOn()
 *
 * @brief   lora reset
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rfTrue = enable lora, rfFalse  - disable lora
 *
 * @return  none
 */
static void SX1276SetLoRaOn( rfBool a_bEnable );


/***************************************************************************************************
 *  EXTERNAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      sx1278_Init()
 *
 * @brief   Rf radio init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_Init( void )
{

    SX1276LR = ( tSX1276LR* )SX1276Regs;
    
    sx1278_DioInit();
    sx1278_SpiInit();
    
    SX1276Reset( );
    
    SX1276SetLoRaOn( rfTrue );
    //RFLRState = RFLR_STATE_IDLE;

    //SX1276LoRaSetDefaults( );
    /* read reg value to buffer registers */
    sx1278_ReadBuffer__( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );
    
    /* @config area */
    /* if use crystal oscillator, RegTcxo.bit4 = 0, else if use external clock, RegTcxo.bit4 = 1 */
    SX1276LR->RegTcxo = 0x09;

    /* @wait for modification, here only need update RegTcxo register */
    sx1278_WriteBuffer__( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );

    // set the RF settings 
    SX1276LoRaSetRFFrequency( LoRaSettings.RFFrequency );
    SX1276LoRaSetSpreadingFactor( LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
    SX1276LoRaSetErrorCoding( LoRaSettings.ErrorCoding );
    SX1276LoRaSetPacketCrcOn( LoRaSettings.CrcOn );
    SX1276LoRaSetSignalBandwidth( LoRaSettings.SignalBw );

    SX1276LoRaSetImplicitHeaderOn( LoRaSettings.ImplicitHeaderOn );

    SX1276LoRaSetPayloadLength( LoRaSettings.PayloadLength );
    SX1276LoRaSetLowDatarateOptimize( rfTrue );
    SX1276LoRaSetPreambleLength(LoRaSettings.u16PreambleLength);
    
    if( LoRaSettings.RFFrequency > 860000000 )
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_RFO );
        SX1276LoRaSetPa20dBm( rfFalse );
        LoRaSettings.Power = 14;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    }
    else
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST );
        SX1276LoRaSetPa20dBm( rfTrue );
        LoRaSettings.Power = 20;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    } 
    SX1276LoRaSetSymbTimeout(0x3FF);   /* set timeout value */
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );

}   /* sx1278_Init() */


/***************************************************************************************************
 * @fn      sx1278_WakeUpInit()
 *
 * @brief   Rf radio wakeup init, quick init
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_WakeUpInit( void )
{    
    sx1278_DioInit();
    sx1278_SpiInit();
}   /* sx1278_WakeUpInit() */


/***************************************************************************************************
 * @fn      sx1278_Stop()
 *
 * @brief   stop Rf, rf goto standby state
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_Stop( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
}   /* Rf_SX1278_Stop() */


volatile rfUint32 time_i = 0;
/***************************************************************************************************
 * @fn      sx1278_RxInit()
 *
 * @brief   Init Rf radio for receive
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_RxInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); /* goto standby mode */
    
    do{
        sx1278_Read__( REG_LR_OPMODE, &SX1276LR->RegOpMode );
    }while( 0x81 != SX1276LR->RegOpMode );
    
    
    sx1276_Dio2EnableInt( rfFalse );
    /* enable interrupt */
    SX1276LR->RegIrqFlagsMask = //RFLR_IRQFLAGS_RXTIMEOUT |
                                //RFLR_IRQFLAGS_RXDONE |
                                //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                RFLR_IRQFLAGS_VALIDHEADER |
                                RFLR_IRQFLAGS_TXDONE |
                                RFLR_IRQFLAGS_CADDONE |
                                //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                RFLR_IRQFLAGS_CADDETECTED;
    sx1278_Write__( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );

    /* set frequence hop options */
    if( LoRaSettings.FreqHopOn == rfTrue )
    {
        SX1276LR->RegHopPeriod = LoRaSettings.HopPeriod;

        sx1278_Read__( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        SX1276LoRaSetRFFrequency( HOPPING_FREQUENCIES(SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK));
    }
    else
    {
        SX1276LR->RegHopPeriod = 255;
    }
    sx1278_Write__( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
    
    /* set interrupt dios */
                                    /* RxDone                    RxTimeout                FhssChangeChannel     Payload Crc Error  */
    SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_10;
                                    // CadDetected               ModeReady
    SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
    sx1278_WriteBuffer__( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );

    
    memset( gs_au8PacketBuffer, 0, ( size_t )RF_BUFFER_SIZE );    /* clear buffer */
    gs_u8PacketSize = 0;
    
    SX1276LoRaSetPreambleLength(LoRaSettings.u16PreambleLength);    /* set preamble length */ 
    if(LoRaSettings.u16PreambleLength > 0x3FF)
    {
        SX1276LoRaSetSymbTimeout((0x3FF) );   /* set timeout value */
    }
    else
    {
        SX1276LoRaSetSymbTimeout(LoRaSettings.u16PreambleLength );   /* set timeout value */
        time_i = 0;
    }
    
    /* goto receive mode */
    if( LoRaSettings.RxSingleOn == rfTrue ) /* Rx single mode */
    {
        sx1276_Dio2EnableInt(rfTrue);
        SX1276LoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
    }
    else /* Rx continuous mode */
    {
        SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxBaseAddr;
        sx1278_Write__( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_RECEIVER );
    }
        
    //gs_u32RoutineTmo = ;

}   /* sx1278_RxInit() */


volatile rfUint32 u32ReceiveTimes = 0;
volatile rfUint32 Dio0Cnt = 0;
volatile rfUint32 Dio3Cnt = 0;
/***************************************************************************************************
 * @fn      sx1278_RxProcess()
 *
 * @brief   Rf radio receive process
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - receive success
 *          RF_P_RUNNING  - receive process is running
 *          RF_P_RX_TMO  - receive timeout
 *          RF_P_RX_CRC_ERR  -received data crc error
 */
RF_P_RET_t sx1278_RxProcess( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    rfUint8 u8OpMode = RFLR_OPMODE_STANDBY;

    /* here, monitor RxDone Event, payload Crc Err Event, Receive Timeout Event, and Fhss event use irq beacuse it should be real time */

    sx1278_Read__( REG_LR_OPMODE, &u8OpMode );
    u8OpMode &= (~RFLR_OPMODE_MASK);
    
    /* if rx timeout event happened */
    if(1 == DIO1)
    {
        /* clear rx timeout irq flag */
        sx1278_Write__( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT  );
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); /* goto standby mode */
        tRet = RF_P_RX_TMO;
    } 
    /* if rx done interrupt */
    else if( DIO0 == 1 ) // RxDone
    {
        Dio0Cnt ++;
        /* Clear rx done Irq flag */
        sx1278_Write__( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE  );
#define _SX1278DEBUG
#ifdef _SX1278DEBUG
        sx1278_Read__( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        if(( 1 == DIO3) || ((SX1276LR->RegHopChannel & 0x40) != 0x40))
        {
#else
        
            /* if payload crc event happened */
        if(1 == DIO3)
        {
#endif
            Dio3Cnt ++;
            /* payload crc error irq and rx done irq will happen at the same time */
            /* clear payload crc error irq flag */
            sx1278_Write__( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR  );
            tRet = RF_P_RX_CRC_ERR;
        }
        else
        {
            {
                rfUint8 rxSnrEstimate;
                sx1278_Read__( REG_LR_PKTSNRVALUE, &rxSnrEstimate );
                if( rxSnrEstimate & 0x80 ) // The SNR sign bit is 1
                {
                    // Invert and divide by 4
                    RxPacketSnrEstimate = ( ( ~rxSnrEstimate + 1 ) & 0xFF ) >> 2;
                    RxPacketSnrEstimate = -RxPacketSnrEstimate;
                }
                else
                {
                    // Divide by 4
                    RxPacketSnrEstimate = ( rxSnrEstimate & 0xFF ) >> 2;
                }
            }


            if( RxPacketSnrEstimate < 0 )
            {
                RxPacketRssiValue = NOISE_ABSOLUTE_ZERO + 10.0 * SignalBwLog[LoRaSettings.SignalBw] + NOISE_FIGURE_LF + ( double )RxPacketSnrEstimate;
            }
            else
            {
                sx1278_Read__( REG_LR_PKTRSSIVALUE, &SX1276LR->RegPktRssiValue );
                RxPacketRssiValue = RssiOffsetLF[LoRaSettings.SignalBw] + ( double )SX1276LR->RegPktRssiValue;
            }
                
            /* Rx single mode */
            if( LoRaSettings.RxSingleOn == rfTrue )
            {
                SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxBaseAddr;
                sx1278_Write__( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );

                if( LoRaSettings.ImplicitHeaderOn == rfTrue )
                {
                    gs_u8PacketSize = SX1276LR->RegPayloadLength;
                    sx1278_ReadFifo__( gs_au8PacketBuffer, SX1276LR->RegPayloadLength );
                }
                else
                {             
                    sx1278_Read__( REG_LR_NBRXBYTES, &SX1276LR->RegNbRxBytes );
                    gs_u8PacketSize = SX1276LR->RegNbRxBytes;                    
                    sx1278_ReadFifo__( gs_au8PacketBuffer, gs_u8PacketSize );
                }
            }
            /* Rx continuous mode */
            else
            {
                sx1278_Read__( REG_LR_FIFORXCURRENTADDR, &SX1276LR->RegFifoRxCurrentAddr );

                if( LoRaSettings.ImplicitHeaderOn == rfTrue )
                {
                    gs_u8PacketSize = SX1276LR->RegPayloadLength;
                    SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxCurrentAddr;
                    sx1278_Write__( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
                    sx1278_ReadFifo__( gs_au8PacketBuffer, SX1276LR->RegPayloadLength );
                }
                else
                {
                    sx1278_Read__( REG_LR_NBRXBYTES, &SX1276LR->RegNbRxBytes );
                    gs_u8PacketSize = SX1276LR->RegNbRxBytes;
                    SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxCurrentAddr;
                    sx1278_Write__( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
                    sx1278_ReadFifo__( gs_au8PacketBuffer, SX1276LR->RegNbRxBytes );
                }
            }
            u32ReceiveTimes ++;
            tRet = RF_P_OK;
        }
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); /* goto standby mode */
        
    }
    
    /* others */
    else
    {
        tRet = RF_P_RUNNING;
    }
    
    
    if( (tRet == RF_P_RUNNING) && (RFLR_OPMODE_STANDBY == u8OpMode) ){
        //tRet = RF_P_UNKOWN_STANDBY;
    }
    
    return tRet;

}   /* sx1278_RxProcess() */

/***************************************************************************************************
 * @fn      sx1278_TxInit()
 *
 * @brief   Init Rf for transmit process
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_TxInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
    
    do{
        sx1278_Read__( REG_LR_OPMODE, &SX1276LR->RegOpMode );
    }while( 0x81 != SX1276LR->RegOpMode );

    if( LoRaSettings.FreqHopOn == rfTrue )
    {
        SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    RFLR_IRQFLAGS_RXDONE |
                                    RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    //RFLR_IRQFLAGS_TXDONE |
                                    RFLR_IRQFLAGS_CADDONE |
                                    //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                    RFLR_IRQFLAGS_CADDETECTED;
        SX1276LR->RegHopPeriod = LoRaSettings.HopPeriod;
        sx1278_Read__( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        SX1276LoRaSetRFFrequency( HOPPING_FREQUENCIES(SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK));
    }
    else
    {
        SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                    RFLR_IRQFLAGS_RXDONE |
                                    RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                    RFLR_IRQFLAGS_VALIDHEADER |
                                    //RFLR_IRQFLAGS_TXDONE |
                                    RFLR_IRQFLAGS_CADDONE |
                                    RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                    RFLR_IRQFLAGS_CADDETECTED;
        SX1276LR->RegHopPeriod = 0;

    }
    
    SX1276LoRaSetPacketCrcOn( LoRaSettings.CrcOn );
    
    sx1278_Write__( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
    sx1278_Write__( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );
    
    SX1276LoRaSetPreambleLength(LoRaSettings.u16PreambleLength);    /* set preamble length */

    // Initializes the payload size
    SX1276LR->RegPayloadLength = gs_u8PacketSize;
    sx1278_Write__( REG_LR_PAYLOADLENGTH, SX1276LR->RegPayloadLength );

    if( LoRaSettings.RFFrequency > 860000000 )
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_RFO );
        SX1276LoRaSetPa20dBm( rfFalse );
        LoRaSettings.Power = 14;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    }
    else
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST );
        SX1276LoRaSetPa20dBm( rfTrue );
        LoRaSettings.Power = 20;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    } 
    
    SX1276LR->RegFifoTxBaseAddr = 0x00; // Full buffer used for Tx
    sx1278_Write__( REG_LR_FIFOTXBASEADDR, SX1276LR->RegFifoTxBaseAddr );

    SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoTxBaseAddr;
    sx1278_Write__( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
    
    // Write payload buffer to LORA modem
    sx1278_WriteFifo__( gs_au8PacketBuffer, gs_u8PacketSize );
    
                                    /* TxDone               RxTimeout                   FhssChangeChannel          ValidHeader         */
    SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_01;
                                    /* PllLock              Mode Ready  */
    SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_01 | RFLR_DIOMAPPING2_DIO5_00;
    sx1278_WriteBuffer__( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );
    
    sx1276_Dio2EnableInt(rfTrue);
    SX1276LoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );

    /* clear Tx Time */
    gs_u32PacketTxTime = 0;
    
}   /* sx1278_TxInit() */


/***************************************************************************************************
 * @fn      sx1278_TxProcess()
 *
 * @brief   Rf radio transmit process
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - transmit success
 *          RF_P_RUNNING  - receive process is running
 *          RF_P_TX_TMO  - receive timeout
 */
RF_P_RET_t sx1278_TxProcess( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    
    //if(gs_u32PacketTxTime > LoRaSettings.TxPacketTimeout)
    {
        //tRet = RF_P_TX_TMO;
    }
  //  else
    {
        if( DIO0 == 1 ) // TxDone
        {
            sx1276_Dio2EnableInt(rfFalse);
            /* Clear txdone Irq flag*/
            sx1278_Write__( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE  );
            /* optimize the power consumption by switching off the transmitter as soon as the packet has been sent */
            SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
            tRet = RF_P_OK;
        }
    }
    return tRet;

}   /* sx1278_TxProcess */


/***************************************************************************************************
 * @fn      sx1278_CADInit()
 *
 * @brief   Init Rf for cad process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_CADInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );

    /* set frequence hop options */
    if( LoRaSettings.FreqHopOn == rfTrue )
    {
        SX1276LR->RegHopPeriod = LoRaSettings.HopPeriod;

        sx1278_Read__( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        SX1276LoRaSetRFFrequency( HOPPING_FREQUENCIES(SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK));
    }
    else
    {
        SX1276LR->RegHopPeriod = 255;
    }
    
    SX1276LR->RegIrqFlagsMask = RFLR_IRQFLAGS_RXTIMEOUT |
                                RFLR_IRQFLAGS_RXDONE |
                                RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                RFLR_IRQFLAGS_VALIDHEADER |
                                RFLR_IRQFLAGS_TXDONE |
                                //RFLR_IRQFLAGS_CADDONE |
                                RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL; // |
                                //RFLR_IRQFLAGS_CADDETECTED;
    sx1278_Write__( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );
    
                                // RxDone                   RxTimeout                   FhssChangeChannel           CadDone
    SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_10 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                // CAD Detected              ModeReady
    SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_01 | RFLR_DIOMAPPING2_DIO5_00;
    sx1278_WriteBuffer__( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );
        
    SX1276LoRaSetOpMode( RFLR_OPMODE_CAD );
    
    gs_u32PacketTxTime = 0;
    
}   /* sx1278_CADInit() */


/***************************************************************************************************
 * @fn      sx1278_CADProcess()
 *
 * @brief   Rf radio CAD process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - CAD success
 *          RF_P_RUNNING  - CAD is running
 *          RF_P_EMPTY  - CAD channel is empty
 */
RF_P_RET_t sx1278_CADProcess( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    
    if(gs_u32PacketTxTime >= 5)
    {
        //tRet = RF_P_CAD_EMPTY;
    }
    
    if( DIO3 == 1 ) //CAD Done interrupt
    { 
        // Clear Irq
        sx1278_Write__( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE  );
        if( DIO1 == 1 ) // CAD Detected interrupt
        {
            // Clear Irq
            sx1278_Write__( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED  );
            // CAD detected, we have a LoRa preamble
            tRet = RF_P_OK;
        } 
        else
        {    
            // The device goes in Standby Mode automatically    
            tRet = RF_P_CAD_EMPTY;
        }
    }

    return tRet;
}   /* sx1278_CADProcess() */

/***************************************************************************************************
 * @fn      sx1278_SleepInit()
 *
 * @brief   Init Rf for sleep process
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void sx1278_SleepInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_SLEEP );
}   /* sx1278_SleepInit() */


/***************************************************************************************************
 * @fn      sx1278_SleepProcess()
 *
 * @brief   Rf radio sleep process
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - transmit success
 *          RF_P_RUNNING  - receive process is running
 */
RF_P_RET_t sx1278_SleepProcess( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    
    sx1278_SpiDeInit();
    sx1278_DioDeInit();
    tRet = RF_P_OK;
    
    return tRet;
}   /* sx1278_SleepProcess() */


/***************************************************************************************************
 * @fn      SX1276FHSSChangeChannel_ISR()
 *
 * @brief   Rf radio Fhss change channel interrupt
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void SX1276FHSSChangeChannel_ISR(void)
{
    sx1278_Write__( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
    if( LoRaSettings.FreqHopOn == rfTrue )
    {
        SX1276LoRaSetSymbTimeout(LoRaSettings.HopPeriod);
        gs_u32PacketTxTime = 0;
        sx1278_Read__( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        SX1276LoRaSetRFFrequency( HOPPING_FREQUENCIES(SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK));
        
    }
    // Clear Irq
    

    // Debug
    //RxGain = SX1276LoRaReadRxGain( );   

}   /* SX1276FHSS_ISR() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetTxPacket()
 *
 * @brief   set tx packet
 *
 * @author	chuanpengl
 *
 * @param   a_pvdBuffer  - data need to be send
 *          a_u8Size  - data size
 *
 * @return  none
 */
void SX1276LoRaSetTxPacket( const void *a_pvdBuffer, rfUint8 a_u8Size )
{
    gs_u8PacketSize = a_u8Size;
    memcpy( ( void * )gs_au8PacketBuffer, a_pvdBuffer, gs_u8PacketSize ); 
}   /* SX1276LoRaSetTxPacket() */


/***************************************************************************************************
 * @fn      sx1278_GetBufferAddr()
 *
 * @brief   get lora buffer address, this buffer is used as tx buffer and rx buffer
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  txbuffer address
 */
void *sx1278_GetBufferAddr(void)
{
    return ( void * ) gs_au8PacketBuffer;
}   /* sx1278_GetBufferAddr() */


/***************************************************************************************************
 * @fn      sx1278_SetPacketSize()
 *
 * @brief   set tx packet size
 *
 * @author  chuanpengl
 *
 * @param   a_u8Size  - tx packet size
 *
 * @return  none
 */
void sx1278_SetPacketSize( rfUint16 a_u16Size )
{
    gs_u8PacketSize = (rfUint8)a_u16Size;
}   /* sx1278_SetPacketSize() */


/***************************************************************************************************
 * @fn      sx1278_GetPacketSize()
 *
 * @brief   get rx packet size
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  rx packet size
 */
rfUint16 sx1278_GetPacketSize( void )
{
    return gs_u8PacketSize;
}   /* sx1278_GetPacketSize() */

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
 

/***************************************************************************************************
 * @fn      SX1276LoRaSetRFBaseFrequency()
 *
 * @brief   set LoRa base frequence, it will be the first frequence when hop
 *
 * @author	chuanpengl
 *
 * @param   a_u32Freq  - rf frequence, unit is Hz
 *
 * @return  none
 */
void SX1276LoRaSetRFBaseFrequency( rfUint32 a_u32Freq)
{
    LoRaSettings.RFFrequency = a_u32Freq;
}   /* SX1276LoRaSetRFBaseFrequency() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetRFFrequency()
 *
 * @brief   set LoRa frequence
 *
 * @author	chuanpengl
 *
 * @param   a_u32Freq  - rf frequence, unit is Hz
 *
 * @return  none
 */
void SX1276LoRaSetRFFrequency( rfUint32 a_u32Freq)
{
    a_u32Freq = ( rfUint32 )( ( double )a_u32Freq / ( double )FREQ_STEP );
    
    SX1276LR->RegFrfMsb = ( rfUint8 )( ( a_u32Freq >> 16 ) & 0xFF );
    SX1276LR->RegFrfMid = ( rfUint8 )( ( a_u32Freq >> 8 ) & 0xFF );
    SX1276LR->RegFrfLsb = ( rfUint8 )( a_u32Freq & 0xFF );

    sx1278_WriteBuffer__( REG_LR_FRFMSB, &SX1276LR->RegFrfMsb, 3 );
}   /* SX1276LoRaSetRFFrequency() */



/***************************************************************************************************
 * @fn      SX1276LoRaGetRFFrequency()
 *
 * @brief   get LoRa frequence
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  frequence, in unit Hz
 */
rfUint32 SX1276LoRaGetRFFrequency( void )
{
    sx1278_ReadBuffer__( REG_LR_FRFMSB, &SX1276LR->RegFrfMsb, 3 );
    LoRaSettings.RFFrequency = ( ( uint32_t )SX1276LR->RegFrfMsb << 16 ) | ( ( uint32_t )SX1276LR->RegFrfMid << 8 ) | ( ( uint32_t )SX1276LR->RegFrfLsb );
    LoRaSettings.RFFrequency = ( uint32_t )( ( double )LoRaSettings.RFFrequency * ( double )FREQ_STEP );

    return LoRaSettings.RFFrequency;
}   /* SX1276LoRaGetRFFrequency() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetRFPower()
 *
 * @brief   set LoRa RF power
 *
 * @author	chuanpengl
 *
 * @param   a_i8Power  - power value
 *
 * @return  none
 */
void SX1276LoRaSetRFPower( rfInt8 a_i8Power )
{
    sx1278_Read__( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
    sx1278_Read__( REG_LR_PADAC, &SX1276LR->RegPaDac );
    
    if( ( SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( SX1276LR->RegPaDac & 0x87 ) == 0x87 )
        {
            if( a_i8Power < 5 )
            {
                a_i8Power = 5;
            }
            if( a_i8Power > 20 )
            {
                a_i8Power = 20;
            }
            SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( rfUint8 )( ( uint16_t )( a_i8Power - 5 ) & 0x0F );
        }
        else
        {
            if( a_i8Power < 2 )
            {
                a_i8Power = 2;
            }
            if( a_i8Power > 17 )
            {
                a_i8Power = 17;
            }
            SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
            SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( rfUint8 )( ( uint16_t )( a_i8Power - 2 ) & 0x0F );
        }
    }
    else
    {
        if( a_i8Power < -1 )
        {
            a_i8Power = -1;
        }
        if( a_i8Power > 14 )
        {
            a_i8Power = 14;
        }
        SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_MAX_POWER_MASK ) | 0x70;
        SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( rfUint8 )( ( uint16_t )( a_i8Power + 1 ) & 0x0F );
    }
    sx1278_Write__( REG_LR_PACONFIG, SX1276LR->RegPaConfig );
    LoRaSettings.Power = a_i8Power;
}   /* SX1276LoRaSetRFPower() */

/***************************************************************************************************
 * @fn      SX1276LoRaGetRFPower()
 *
 * @brief   get LoRa RF power 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rf power
 */
rfInt8 SX1276LoRaGetRFPower( void )
{
    sx1278_Read__( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
    sx1278_Read__( REG_LR_PADAC, &SX1276LR->RegPaDac );

    if( ( SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {
        if( ( SX1276LR->RegPaDac & 0x07 ) == 0x07 )
        {
            LoRaSettings.Power = 5 + ( SX1276LR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
        else
        {
            LoRaSettings.Power = 2 + ( SX1276LR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
        }
    }
    else
    {
        LoRaSettings.Power = -1 + ( SX1276LR->RegPaConfig & ~RFLR_PACONFIG_OUTPUTPOWER_MASK );
    }
    return LoRaSettings.Power;
}   /* SX1276LoRaGetRFPower() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetSignalBandwidth()
 *
 * @brief   Set LoRa bandwidth
 *
 * @author	chuanpengl
 *
 * @param   a_u8Bw  - bandwidth
 *
 * @return  none
 */
void SX1276LoRaSetSignalBandwidth( rfUint8 a_u8Bw )
{
    sx1278_Read__( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    SX1276LR->RegModemConfig1 = ( SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_BW_MASK ) | ( a_u8Bw << 4 );
    sx1278_Write__( REG_LR_MODEMCONFIG1, SX1276LR->RegModemConfig1 );
    LoRaSettings.SignalBw = a_u8Bw;
}   /* SX1276LoRaSetSignalBandwidth() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetSignalBandwidth()
 *
 * @brief   Get LoRa Bandwidth 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  get LoRa signal bandwidth
 */
rfUint8 SX1276LoRaGetSignalBandwidth( void )
{
    sx1278_Read__( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    LoRaSettings.SignalBw = ( SX1276LR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_BW_MASK ) >> 4;
    return LoRaSettings.SignalBw;
}   /* SX1276LoRaGetSignalBandwidth() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetSpreadingFactor()
 *
 * @brief   set LoRa spreading factor
 *
 * @author	chuanpengl
 *
 * @param   a_u8Factor  - spreading factor 
 *
 * @return  none
 */
void SX1276LoRaSetSpreadingFactor( rfUint8 a_u8Factor )
{

    if( a_u8Factor > 12 )
    {
        a_u8Factor = 12;
    }
    else if( a_u8Factor < 6 )
    {
        a_u8Factor = 6;
    }

    if( a_u8Factor == 6 )
    {
        SX1276LoRaSetNbTrigPeaks( 5 );
    }
    else
    {
        SX1276LoRaSetNbTrigPeaks( 3 );
    }

    sx1278_Read__( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );    
    SX1276LR->RegModemConfig2 = ( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_SF_MASK ) | ( a_u8Factor << 4 );
    sx1278_Write__( REG_LR_MODEMCONFIG2, SX1276LR->RegModemConfig2 );    
    LoRaSettings.SpreadingFactor = a_u8Factor;
}   /* SX1276LoRaSetSpreadingFactor() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetSpreadingFactor()
 *
 * @brief   Get LoRa spreading factor
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  spreading factor
 */
rfUint8 SX1276LoRaGetSpreadingFactor( void )
{
    sx1278_Read__( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );   
    LoRaSettings.SpreadingFactor = ( SX1276LR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SF_MASK ) >> 4;
    return LoRaSettings.SpreadingFactor;
}   /* SX1276LoRaGetSpreadingFactor() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetErrorCoding()
 *
 * @brief   set LoRa Error Coding value 
 *
 * @author	author name
 *
 * @param   a_u8Value  - Error Coding value
 *
 * @return  none
 */
void SX1276LoRaSetErrorCoding( rfUint8 a_u8Value )
{
    sx1278_Read__( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    SX1276LR->RegModemConfig1 = ( SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( a_u8Value << 1 );
    sx1278_Write__( REG_LR_MODEMCONFIG1, SX1276LR->RegModemConfig1 );
    LoRaSettings.ErrorCoding = a_u8Value;
}   /* SX1276LoRaSetErrorCoding() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetErrorCoding()
 *
 * @brief   Get LoRa Error Coding Value 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  LoRa Error Coding Value
 */
rfUint8 SX1276LoRaGetErrorCoding( void )
{
    sx1278_Read__( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    LoRaSettings.ErrorCoding = ( SX1276LR->RegModemConfig1 & ~RFLR_MODEMCONFIG1_CODINGRATE_MASK ) >> 1;
    return LoRaSettings.ErrorCoding;
}   /* SX1276LoRaGetErrorCoding() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetPacketCrcOn()
 *
 * @brief   enable crc or not
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rfTrue = enable crc, rfFalse = disable
 *
 * @return  none
 */
void SX1276LoRaSetPacketCrcOn( rfBool a_bEnable )
{
    sx1278_Read__( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );
    SX1276LR->RegModemConfig2 = ( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( a_bEnable << 2 );
    sx1278_Write__( REG_LR_MODEMCONFIG2, SX1276LR->RegModemConfig2 );
    LoRaSettings.CrcOn = a_bEnable;
}   /* SX1276LoRaSetPacketCrcOn() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetPacketCrcOn()
 *
 * @brief   check crc is on or not
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rfTrue  - crc is on
 *          rfFalse  - crc is off
 */
rfBool SX1276LoRaGetPacketCrcOn( void )
{
    sx1278_Read__( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );
    LoRaSettings.CrcOn = (( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_ON ) >> 1) > 0 ? rfTrue : rfFalse;
    return LoRaSettings.CrcOn;
}   /* SX1276LoRaGetPacketCrcOn */


/***************************************************************************************************
 * @fn      SX1276LoRaSetPreambleLength()
 *
 * @brief   set LoRa preamble length
 *
 * @author	chuanpengl
 *
 * @param   a_u16Value  - preamble length value
 *
 * @return  none
 */
void SX1276LoRaSetPreambleLength( uint16_t a_u16Value )
{
    sx1278_ReadBuffer__( REG_LR_PREAMBLEMSB, &SX1276LR->RegPreambleMsb, 2 );

    SX1276LR->RegPreambleMsb = ( a_u16Value >> 8 ) & 0x00FF;
    SX1276LR->RegPreambleLsb = a_u16Value & 0xFF;
    sx1278_WriteBuffer__( REG_LR_PREAMBLEMSB, &SX1276LR->RegPreambleMsb, 2 );
}   /* SX1276LoRaSetPreambleLength() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetPreambleLength()
 *
 * @brief   get LoRa preamble length
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  preamble length value
 */
uint16_t SX1276LoRaGetPreambleLength( void )
{
    sx1278_ReadBuffer__( REG_LR_PREAMBLEMSB, &SX1276LR->RegPreambleMsb, 2 );
    return ( ( SX1276LR->RegPreambleMsb & 0x00FF ) << 8 ) | SX1276LR->RegPreambleLsb;
}   /* SX1276LoRaGetPreambleLength() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetImplicitHeaderOn()
 *
 * @brief   enable implicit header or not
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rfTrue = enable implicit header, rfFalse = disable implicit header
 *
 * @return  none
 */
void SX1276LoRaSetImplicitHeaderOn( rfBool a_bEnable )
{
    sx1278_Read__( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    SX1276LR->RegModemConfig1 = ( SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( a_bEnable );
    sx1278_Write__( REG_LR_MODEMCONFIG1, SX1276LR->RegModemConfig1 );
    LoRaSettings.ImplicitHeaderOn = a_bEnable;
}   /* SX1276LoRaSetImplicitHeaderOn() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetImplicitHeaderOn()
 *
 * @brief   check implicit header is on or not
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rfTrue  - implicit header is on
 *          rfFalse  - implicit header is off
 */
rfBool SX1276LoRaGetImplicitHeaderOn( void )
{
    sx1278_Read__( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    LoRaSettings.ImplicitHeaderOn = ((SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_ON)) > 0 ?\
      rfTrue : rfFalse;
    return LoRaSettings.ImplicitHeaderOn;
}   /* SX1276LoRaGetImplicitHeaderOn */


/***************************************************************************************************
 * @fn      SX1276LoRaSetRxSingleOn()
 *
 * @brief   enable rx single mode 
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rfTrue = rx single on, rfFalse = rx continuous mode 
 *
 * @return  none
 */
void SX1276LoRaSetRxSingleOn( rfBool a_bEnable )
{
    LoRaSettings.RxSingleOn = a_bEnable;
}   /* SX1276LoRaSetRxSingleOn() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetRxSingleOn()
 *
 * @brief   check whether single receive mode or continus receive mode 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rfTrue  - rx single on
 *          rfFalse  - rx continuous
 */
rfBool SX1276LoRaGetRxSingleOn( void )
{
    return LoRaSettings.RxSingleOn;
}   /* SX1276LoRaGetRxSingleOn() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetFreqHopOn()
 *
 * @brief   enable/disable freq hop function 
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rfTrue = enable, rfFalse = disable 
 *
 * @return  none
 */
void SX1276LoRaSetFreqHopOn( rfBool a_bEnable )
{
    LoRaSettings.FreqHopOn = a_bEnable;
}


/***************************************************************************************************
 * @fn      SX1276LoRaGetFreqHopOn()
 *
 * @brief   check whether hop is enable or not
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  rfTrue  - hop is enable
 *          rfFalse  - hop is disable
 */
rfBool SX1276LoRaGetFreqHopOn( void )
{
    return LoRaSettings.FreqHopOn;
}   /* SX1276LoRaGetFreqHopOn() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetHopPeriod()
 *
 * @brief   set LoRa hop period
 *
 * @author	chuanpengl
 *
 * @param   a_u8Value  - hop period, symbol counts 
 *
 * @return  none
 */
void SX1276LoRaSetHopPeriod( rfUint8 a_u8Value )
{
    SX1276LR->RegHopPeriod = a_u8Value;
    sx1278_Write__( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
    LoRaSettings.HopPeriod = a_u8Value;
}   /* SX1276LoRaSetHopPeriod() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetHopPeriod()
 *
 * @brief   get LoRa hop period 
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  hop period
 */
rfUint8 SX1276LoRaGetHopPeriod( void )
{
    sx1278_Read__( REG_LR_HOPPERIOD, &SX1276LR->RegHopPeriod );
    LoRaSettings.HopPeriod = SX1276LR->RegHopPeriod;
    return LoRaSettings.HopPeriod;
}   /* SX1276LoRaGetHopPeriod() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetTxPacketTimeout()
 *
 * @brief   Set LoRa Tx Packet timeout time 
 *
 * @author	chuanpengl
 *
 * @param   a_u32Value  - tx timeout time in unit ms 
 *
 * @return  none
 */
void SX1276LoRaSetTxPacketTimeout( uint32_t value )
{
    LoRaSettings.TxPacketTimeout = value;
}   /* SX1276LoRaSetTxPacketTimeout() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetTxPacketTimeout()
 *
 * @brief   Get TxPacket Timeout time 
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  tx packet timeout time in unit ms
 */
uint32_t SX1276LoRaGetTxPacketTimeout( void )
{
    return LoRaSettings.TxPacketTimeout;
}   /* SX1276LoRaGetTxPacketTimeout() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetRxPacketTimeout()
 *
 * @brief   Set LoRa Rx Packet timeout time 
 *
 * @author	chuanpengl
 *
 * @param   a_u32Value  - rx timeout time in unit ms 
 *
 * @return  none
 */
void SX1276LoRaSetRxPacketTimeout( uint32_t a_u32Value )
{
    LoRaSettings.RxPacketTimeout = a_u32Value;
}   /* SX1276LoRaSetRxPacketTimeout() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetRxPacketTimeout()
 *
 * @brief   Get RxPacket Timeout time 
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  rx packet timeout time in unit ms
 */
uint32_t SX1276LoRaGetRxPacketTimeout( void )
{
    return LoRaSettings.RxPacketTimeout;
}   /* SX1276LoRaGetRxPacketTimeout() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetPayloadLength()
 *
 * @brief   set LoRa payload Length 
 *
 * @author	chuanpengl
 *
 * @param   a_u8Value  - payload length in byte 
 *
 * @return  none
 */
void SX1276LoRaSetPayloadLength( rfUint8 a_u8Value )
{
    SX1276LR->RegPayloadLength = a_u8Value;
    sx1278_Write__( REG_LR_PAYLOADLENGTH, SX1276LR->RegPayloadLength );
    LoRaSettings.PayloadLength = a_u8Value;
}   /* SX1276LoRaSetPayloadLength() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetPayloadLength()
 *
 * @brief   get LoRa payload length 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  payload length
 */
rfUint8 SX1276LoRaGetPayloadLength( void )
{
    sx1278_Read__( REG_LR_PAYLOADLENGTH, &SX1276LR->RegPayloadLength );
    LoRaSettings.PayloadLength = SX1276LR->RegPayloadLength;
    return LoRaSettings.PayloadLength;
}   /* SX1276LoRaGetPayloadLength() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetPa20dBm()
 *
 * @brief   enable +20dBm output, if PABOOST is not select, this setting will not active.
 *
 * @author	chuanpengl
 *
 * @param   rfTrue  - enable +20dBm output
 *          rfFalse  - disable +20dBm output
 *
 * @return  none
 */
void SX1276LoRaSetPa20dBm( rfBool enale )
{
    sx1278_Read__( REG_LR_PADAC, &SX1276LR->RegPaDac );
    sx1278_Read__( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );

    if( ( SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {    
        if( enale == rfTrue )
        {
            SX1276LR->RegPaDac = 0x87;
        }
    }
    else
    {
        SX1276LR->RegPaDac = 0x84;
    }
    sx1278_Write__( REG_LR_PADAC, SX1276LR->RegPaDac );
}   /* SX1276LoRaSetPa20dBm() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetPa20dBm()
 *
 * @brief   check whether enable +20dBm or not
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rfTrue  - +20dBm output is enable
 *          rfFalse  - +20dBm output is disable
 */
rfBool SX1276LoRaGetPa20dBm( void )
{
    sx1278_Read__( REG_LR_PADAC, &SX1276LR->RegPaDac );
    
    return ( ( SX1276LR->RegPaDac & 0x07 ) == 0x07 ) ? rfTrue : rfFalse;
}   /* SX1276LoRaGetPa20dBm() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetPAOutput()
 *
 * @brief   set LoRa PA output pin
 *
 * @author	chuanpengl
 *
 * @param   a_u8OutputPin  - 0x00, output with RFO pin, output power is limited to +14dBm.
 *                         - 0x80, output with PA_BOOST pin, output power is limited to +20dBm.
 *                         - other value, is invalid
 *
 * @return  none
 */
void SX1276LoRaSetPAOutput( rfUint8 a_u8OutputPin )
{
    sx1278_Read__( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
    SX1276LR->RegPaConfig = (SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_MASK ) | a_u8OutputPin;
    sx1278_Write__( REG_LR_PACONFIG, SX1276LR->RegPaConfig );
}


/***************************************************************************************************
 * @fn      SX1276LoRaGetPAOutput()
 *
 * @brief   get LoRa PA output pin 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  0x00  - output with RFO pin, output power is limited to +14dBm.
 *          0x80  - output with PA_BOOST pin, output power is limited to +20dBm.
 */
rfUint8 SX1276LoRaGetPAOutput( void )
{
    sx1278_Read__( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
    return SX1276LR->RegPaConfig & ~RFLR_PACONFIG_PASELECT_MASK;
}   /* SX1276LoRaGetPAOutput() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetPaRamp()
 *
 * @brief   set LoRa Ramp time
 *
 * @author	chuanpengl
 *
 * @param   a_u8Value  - [0..15], Ramp up/down time, please reference to manual 
 *
 * @return  none
 */
void SX1276LoRaSetPaRamp( rfUint8 a_u8Value )
{
    sx1278_Read__( REG_LR_PARAMP, &SX1276LR->RegPaRamp );
    SX1276LR->RegPaRamp = ( SX1276LR->RegPaRamp & RFLR_PARAMP_MASK ) | ( a_u8Value & ~RFLR_PARAMP_MASK );
    sx1278_Write__( REG_LR_PARAMP, SX1276LR->RegPaRamp );
}   /* SX1276LoRaSetPaRamp() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetPaRamp()
 *
 * @brief   get LoRa Ramp time 
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  Ramp up/down time, please reference to manual 
 */
rfUint8 SX1276LoRaGetPaRamp( void )
{
    sx1278_Read__( REG_LR_PARAMP, &SX1276LR->RegPaRamp );
    return SX1276LR->RegPaRamp & ~RFLR_PARAMP_MASK;
}   /* SX1276LoRaGetPaRamp() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetSymbTimeout()
 *
 * @brief   set symbol time
 *
 * @author	chuanpengl
 *
 * @param   a_u16Value  - symbol time 
 *
 * @return  none
 */
void SX1276LoRaSetSymbTimeout( rfUint16 a_u16Value )
{
    sx1278_ReadBuffer__( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2, 2 );

    SX1276LR->RegModemConfig2 = ( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( a_u16Value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
    SX1276LR->RegSymbTimeoutLsb = a_u16Value & 0xFF;
    sx1278_WriteBuffer__( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2, 2 );
}   /* SX1276LoRaSetSymbTimeout() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetSymbTimeout()
 *
 * @brief   get symbol time
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  none
 */
rfUint16 SX1276LoRaGetSymbTimeout( void )
{
    sx1278_ReadBuffer__( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2, 2 );
    return ( ( SX1276LR->RegModemConfig2 & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) << 8 ) | SX1276LR->RegSymbTimeoutLsb;
}   /* SX1276LoRaGetSymbTimeout() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetLowDatarateOptimize()
 *
 * @brief   write low data rate optimze
 *
 * @author	author name
 *
 * @param   a_bEnable  - enable low data rate optimeize
 *
 * @return  none
 */
void SX1276LoRaSetLowDatarateOptimize( rfBool a_bEnable )
{
    sx1278_Read__( REG_LR_MODEMCONFIG3, &SX1276LR->RegModemConfig3 );
    SX1276LR->RegModemConfig3 = ( SX1276LR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( a_bEnable << 3 );
    sx1278_Write__( REG_LR_MODEMCONFIG3, SX1276LR->RegModemConfig3 );
}/* SX1276LoRaSetLowDatarateOptimize() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetLowDatarateOptimize()
 *
 * @brief   get low data rate optimize status
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  none
 */
rfBool SX1276LoRaGetLowDatarateOptimize( void )
{
    sx1278_Read__( REG_LR_MODEMCONFIG3, &SX1276LR->RegModemConfig3 );
    return (( SX1276LR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_ON ) >> 3 ) > 0 ? rfTrue : rfFalse;
}/* SX1276LoRaGetLowDatarateOptimize() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetNbTrigPeaks()
 *
 * @brief   set lora nb triger peaks 
 *
 * @author	chuanpengl
 *
 * @param   a_u8Data  - a byte need to be written to SPI controller 
 *
 * @return  none
 */
void SX1276LoRaSetNbTrigPeaks( rfUint8 a_u8Value )
{
    sx1278_Read__( 0x31, &SX1276LR->RegTestReserved31 );
    SX1276LR->RegTestReserved31 = ( SX1276LR->RegTestReserved31 & 0xF8 ) | a_u8Value;
    sx1278_Write__( 0x31, SX1276LR->RegTestReserved31 );
}/* SX1276LoRaSetNbTrigPeaks() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetNbTrigPeaks()
 *
 * @brief   get lora nb triger peaks 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  return get value
 */
rfUint8 SX1276LoRaGetNbTrigPeaks( void )
{
    sx1278_Read__( 0x31, &SX1276LR->RegTestReserved31 );
    return ( SX1276LR->RegTestReserved31 & 0x07 );
}/* SX1276LoRaGetNbTrigPeaks() */
 

/***************************************************************************************************
 * @fn      SX1276LoRaSetOpMode()
 *
 * @brief   set lora operation mode 
 *
 * @author  chuanpengl
 *
 * @param   a_u8OpMode  - operation mode
 *
 * @return  none
 */
void SX1276LoRaSetOpMode( rfUint8 a_u8OpMode )
{
    if( a_u8OpMode == RFLR_OPMODE_TRANSMITTER )
    {
        RXTX( rfTrue );
    }
    
    if(a_u8OpMode == RFLR_OPMODE_RECEIVER_SINGLE)
    {
        RXTX( rfFalse );
    }

    SX1276LR->RegOpMode = ( SX1276LR->RegOpMode & RFLR_OPMODE_MASK ) | a_u8OpMode;
    sx1278_Write__( REG_LR_OPMODE, SX1276LR->RegOpMode );
}


/***************************************************************************************************
 * @fn      SX1276LoRaSetOpMode()
 *
 * @brief   get lora operation mode 
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
rfUint8 SX1278LoRaGetOpMode( void )
{
    sx1278_Read__( REG_LR_OPMODE, &SX1276LR->RegOpMode );
    
    return (SX1276LR->RegOpMode & (~RFLR_OPMODE_MASK));
}

/***************************************************************************************************
 * @fn      SX1276Reset()
 *
 * @brief   lora reset
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void SX1276Reset( void )
{
    uint32_t startTick;
    sx1278_DioReset( rfTrue );
    
    // Wait 1ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 1 ) );    

    sx1278_DioReset( rfFalse );
    
    // Wait 6ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 6 ) );    
}   /* SX1276Reset() */

/***************************************************************************************************
 * @fn      SX1276SetLoRaOn()
 *
 * @brief   lora reset
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rfTrue = enable lora, rfFalse  - disable lora
 *
 * @return  none
 */
void SX1276SetLoRaOn( rfBool a_bEnable )
{
    if( a_bEnable == rfTrue )
    {
        SX1276LoRaSetOpMode( RFLR_OPMODE_SLEEP );
        
        SX1276LR->RegOpMode = ( SX1276LR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON;
        sx1278_Write__( REG_LR_OPMODE, SX1276LR->RegOpMode );
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
                                        // RxDone               RxTimeout                   FhssChangeChannel           CadDone
        SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                        // CadDetected          ModeReady
        SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
        sx1278_WriteBuffer__( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );

        sx1278_ReadBuffer__( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );
    }
    else
    {
        SX1276LoRaSetOpMode( RFLR_OPMODE_SLEEP );
        
        SX1276LR->RegOpMode = ( SX1276LR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF;
        sx1278_Write__( REG_LR_OPMODE, SX1276LR->RegOpMode );
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
        
        sx1278_ReadBuffer__( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );
    }
}   /* SX1276SetLoRaOn() */


/***************************************************************************************************
 * @fn      Rf_Sx1276_UpdateTxPacketTime()
 *
 * @brief   update tx time, called every milisecond
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_Sx1276_UpdateTxPacketTime(void)
{
    gs_u32PacketTxTime ++;
}   /* Rf_Sx1276_UpdateTxPacketTime() */

/***************************************************************************************************
 * @fn      Rf_Sx1276_SetPreambleLengthPara()
 *
 * @brief   set preamble length for LoRaSettings parameter
 *
 * @author	chuanpengl
 *
 * @param   a_u16Length  - paramble length
 *
 * @return  none
 */
void Rf_Sx1276_SetPreambleLengthPara(rfUint16 a_u16length)
{
    LoRaSettings.u16PreambleLength = a_u16length;
}   /* Rf_Sx1276_SetPreambleLengthPara() */

rfInt8 SX1276LoRaGetPacketSnr( void )
{
    return RxPacketSnrEstimate;
}

double SX1276LoRaGetPacketRssi( void )
{
    return RxPacketRssiValue;
}





/***************************************************************************************************
 * @fn      sx1278_Write__()
 *
 * @brief   sx1278 write data to addr
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - write address
 *          a_u8Data  - write data
 *
 * @return  none
 */
void sx1278_Write__( rfUint8 a_u8Addr, rfUint8 a_u8Data )
{
    rfUint8 u8Data[2] = { a_u8Addr | 0x80, a_u8Data };
    sx1278_SpiSetNssHigh(rfFalse);  /* nss low */
    sx1278_SpiWrite( u8Data, 2 );    /* write addr, data */
    sx1278_SpiSetNssHigh(rfTrue);  /* nss high */
}

/***************************************************************************************************
 * @fn      sx1278_Read__()
 *
 * @brief   sx1278 read data from address
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - read address
 *          a_pu8Data  - read data
 *
 * @return  none
 */
void sx1278_Read__( rfUint8 a_u8Addr, rfUint8 *a_pu8Data )
{
    rfUint8 u8Data = a_u8Addr & 0x7F;
    sx1278_SpiSetNssHigh(rfFalse);  /* nss low */
    sx1278_SpiWrite( &u8Data, 1 );    /* write addr */
    sx1278_SpiRead( a_pu8Data, 1 );  /* read data */
    sx1278_SpiSetNssHigh(rfTrue);  /* nss high */
}

/***************************************************************************************************
 * @fn      sx1278_WriteBuffer__()
 *
 * @brief   sx1278 write data to buffer
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - write address
 *          a_pu8Buffer  - write data
 *          a_u16Size  - write data size
 *
 * @return  none
 */
void sx1278_WriteBuffer__( rfUint8 a_u8Addr, rfUint8 *a_pu8Buffer, rfUint16 a_u16Size )
{
    rfUint8 u8Data = a_u8Addr | 0x80 ;
    sx1278_SpiSetNssHigh(rfFalse);  /* nss low */
    sx1278_SpiWrite( &u8Data, 1 );    /* write addr */
    sx1278_SpiWrite( a_pu8Buffer, a_u16Size );    /* write data */
    sx1278_SpiSetNssHigh(rfTrue);  /* nss high */
}

/***************************************************************************************************
 * @fn      sx1278_ReadBuffer__()
 *
 * @brief   sx1278 read data from buffer
 *
 * @author  chuanpengl
 *
 * @param   a_u8Addr  - read address
 *          a_pu8Buffer  - read data
 *          a_u16Size  - read data size
 *
 * @return  none
 */
void sx1278_ReadBuffer__( rfUint8 a_u8Addr, rfUint8 *a_pu8Buffer, rfUint16 a_u16Size )
{
    rfUint8 u8Data = a_u8Addr & 0x7F ;
    sx1278_SpiSetNssHigh(rfFalse);  /* nss low */
    sx1278_SpiWrite( &u8Data, 1 );    /* write addr */
    sx1278_SpiRead( a_pu8Buffer, a_u16Size );    /* read data */
    sx1278_SpiSetNssHigh(rfTrue);  /* nss high */
}

/***************************************************************************************************
 * @fn      sx1278_WriteFifo__()
 *
 * @brief   sx1278 write data to fifo
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Buffer  - write data
 *          a_u16Size  - write data size
 *
 * @return  none
 */
void sx1278_WriteFifo__( rfUint8 *a_pu8Buffer, rfUint16 a_u16Size )
{
    sx1278_WriteBuffer__( 0, a_pu8Buffer, a_u16Size );
}

/***************************************************************************************************
 * @fn      sx1278_ReadFifo__()
 *
 * @brief   sx1278 read data from fifo
 *
 * @author  chuanpengl
 *
 * @param   a_pu8Buffer  - read data
 *          a_u16Size  - read data size
 *
 * @return  none
 */
void sx1278_ReadFifo__( uint8_t *a_pu8Buffer, uint8_t a_u16Size )
{
    sx1278_ReadBuffer__( 0, a_pu8Buffer, a_u16Size );
}

#endif /* (BAL_USE_SX1278 == BAL_MODULE_ON) */

/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150708
*   context: here write modified history
*
***************************************************************************************************/
