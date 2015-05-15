/***************************************************************************************************
* @file name	rf_sx1278.c
* @data   		2014/11/17
* @auther   	chuanpengl
* @module   	sx1278 hal
* @brief 		sx1278 process
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include <string.h>
#include "../common/rf_config.h"
#include "sx1276_hal.h"
#include "sx1276_spi.h"
#include "sx1276_lora.h"
#include "systick/hal_systick.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */
#define _SX1278DEBUG
/***************************************************************************************************
 * MACROS
 */
#define XTAL_FREQ                                   32000000
#define FREQ_STEP                                   61.03515625

#define HOPPING_FREQUENCIES(IDX)    (HoppingFrequenciesDelta[IDX] + LoRaSettings.RFFrequency)
#define GET_TICK_COUNT()                            systick_Get()
#define TICK_RATE_MS( ms )                          ( ms )
/***************************************************************************************************
 * CONSTANTS
 */
/*!
 * Frequency hopping frequencies table
 */
static const rf_int32 HoppingFrequenciesDelta[] =
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
	450000>>1,
	550000>>1,
	650000>>1,
	750000>>1,
	850000>>1,
	950000>>1,
	900000>>1,
	800000>>1,
	700000>>1,
	600000>>1,
	500000>>1,
	400000>>1

};


/***************************************************************************************************
 * TYPEDEFS
 */

/* define irq flags */
typedef struct
{
    rf_uint8 bitRxTimeout:1;
    rf_uint8 bitRxDone:1;
    rf_uint8 bitPayloadCrcErr:1;
    rf_uint8 bitValid:1;
    rf_uint8 bitTxDne:1;
    rf_uint8 bitCadDone:1;
    rf_uint8 bitFhssChangeChn:1;
    rf_uint8 bitCadDetected:1;
}SX1278_IRQ_FLAGS_t;



/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */

/* SX1276 registers variable */
static rf_uint8 SX1276Regs[0x70] = {0};

tSX1276LR* SX1276LR;
// Default settings
tLoRaSettings LoRaSettings =
{
    430000000,        // RFFrequency
    20,               // Power
    8,                // SignalBw [0: 7.8kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                      // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]
    7,                // SpreadingFactor [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    3,                // ErrorCoding [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    rf_true,             // CrcOn [0: OFF, 1: ON]
    rf_false,            // ImplicitHeaderOn [0: OFF, 1: ON]
    rf_true,             // RxSingleOn [0: Continuous, 1 Single]
    rf_true,            // FreqHopOn [0: OFF, 1: ON]
    4,                // HopPeriod Hops every frequency hopping period symbols
    2000,              // TxPacketTimeout
    /* here, if in cad mode, rx timeout value may be need greater then sleep time */
    20,              // RxPacketTimeout
    128,              // PayloadLength (used for implicit header mode)
    12,              /* preamble length */
};


//static SX1278_IRQ_FLAGS_t gs_tIrqFlags = {0};

static rf_uint8 gs_u8PacketSize = 0;
static rf_uint8 gs_au8PacketBuffer[RF_BUFFER_SIZE] = {0};

static rf_uint32 gs_u32PacketTxTime = 0;

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
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
 * @param   a_bEnable  - rf_true = enable lora, rf_false  - disable lora
 *
 * @return  none
 */
static void SX1276SetLoRaOn( rf_bool a_bEnable );


/***************************************************************************************************
 *  EXTERNAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      Rf_SX1278_Init()
 *
 * @brief   Rf radio init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_SX1278_Init( void )
{

    SX1276LR = ( tSX1276LR* )SX1276Regs;
    
    SX1276InitIo( );
    sx1276_spi_init();
    
    SX1276Reset( );
    
    SX1276SetLoRaOn( rf_true );
    //RFLRState = RFLR_STATE_IDLE;

    //SX1276LoRaSetDefaults( );
    
    /* read reg value to buffer registers */
    SX1276ReadBuffer( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );
    
    /* @config area */
    /* if use crystal oscillator, RegTcxo.bit4 = 0, else if use external clock, RegTcxo.bit4 = 1 */
    SX1276LR->RegTcxo = 0x09;

    /* @wait for modification, here only need update RegTcxo register */
    SX1276WriteBuffer( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );

    // set the RF settings 
    SX1276LoRaSetRFFrequency( LoRaSettings.RFFrequency );
    SX1276LoRaSetSpreadingFactor( LoRaSettings.SpreadingFactor ); // SF6 only operates in implicit header mode.
    SX1276LoRaSetErrorCoding( LoRaSettings.ErrorCoding );
    SX1276LoRaSetPacketCrcOn( LoRaSettings.CrcOn );
    SX1276LoRaSetSignalBandwidth( LoRaSettings.SignalBw );

    SX1276LoRaSetImplicitHeaderOn( LoRaSettings.ImplicitHeaderOn );

    SX1276LoRaSetPayloadLength( LoRaSettings.PayloadLength );
    SX1276LoRaSetLowDatarateOptimize( rf_true );
    SX1276LoRaSetPreambleLength(LoRaSettings.u16PreambleLength);
    
    if( LoRaSettings.RFFrequency > 860000000 )
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_RFO );
        SX1276LoRaSetPa20dBm( rf_false );
        LoRaSettings.Power = 14;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    }
    else
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST );
        SX1276LoRaSetPa20dBm( rf_true );
        LoRaSettings.Power = 20;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    } 
    SX1276LoRaSetSymbTimeout(0x3FF);   /* set timeout value */
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );

}   /* Rf_SX1278_Init() */


/***************************************************************************************************
 * @fn      Rf_SX1278_WakeUpInit()
 *
 * @brief   Rf radio wakeup init, quick init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_SX1278_WakeUpInit( void )
{    
    SX1276InitIo( );
    sx1276_spi_init();
}   /* Rf_SX1278_WakeUpInit() */

/***************************************************************************************************
 * @fn      Rf_SX1278_Stop()
 *
 * @brief   stop Rf, rf goto standby state
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_SX1278_Stop( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
}   /* Rf_SX1278_Stop() */

/***************************************************************************************************
 * @fn      Rf_SX1278_RxInit()
 *
 * @brief   Init Rf radio for receive
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_SX1278_RxInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); /* goto standby mode */
    sx1276_EnDio2Int(0);
    /* enable interrupt */
    SX1276LR->RegIrqFlagsMask = //RFLR_IRQFLAGS_RXTIMEOUT |
                                //RFLR_IRQFLAGS_RXDONE |
                                //RFLR_IRQFLAGS_PAYLOADCRCERROR |
                                RFLR_IRQFLAGS_VALIDHEADER |
                                RFLR_IRQFLAGS_TXDONE |
                                RFLR_IRQFLAGS_CADDONE |
                                //RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL |
                                RFLR_IRQFLAGS_CADDETECTED;
    SX1276Write( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );

    /* set frequence hop options */
    if( LoRaSettings.FreqHopOn == rf_true )
    {
        SX1276LR->RegHopPeriod = LoRaSettings.HopPeriod;

        SX1276Read( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        SX1276LoRaSetRFFrequency( HOPPING_FREQUENCIES(SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK));
    }
    else
    {
        SX1276LR->RegHopPeriod = 255;
    }
    SX1276Write( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
    
    /* set interrupt dios */
                                    /* RxDone                    RxTimeout                FhssChangeChannel     Payload Crc Error  */
    SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_10;
                                    // CadDetected               ModeReady
    SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
    SX1276WriteBuffer( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );

    
    memset( gs_au8PacketBuffer, 0, ( size_t )RF_BUFFER_SIZE );    /* clear buffer */
    gs_u8PacketSize = 0;
    
    SX1276LoRaSetPreambleLength(LoRaSettings.u16PreambleLength);    /* set preamble length */ 
    if(LoRaSettings.u16PreambleLength + 5 > 0x3FF)
    {
        SX1276LoRaSetSymbTimeout((0x3FF) );   /* set timeout value */
    }
    else
    {
        SX1276LoRaSetSymbTimeout((LoRaSettings.u16PreambleLength + 5) );   /* set timeout value */
    }
    
    /* goto receive mode */
    if( LoRaSettings.RxSingleOn == rf_true ) /* Rx single mode */
    {
        sx1276_EnDio2Int(1);
        SX1276LoRaSetOpMode( RFLR_OPMODE_RECEIVER_SINGLE );
    }
    else /* Rx continuous mode */
    {
        SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxBaseAddr;
        SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_RECEIVER );
    }
        
    //gs_u32RoutineTmo = ;

}   /* Rf_SX1278_RxInit() */




/***************************************************************************************************
 * @fn      Rf_SX1278_RxProcess()
 *
 * @brief   Rf radio receive process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - receive success
 *          RF_P_RUNNING  - receive process is running
 *          RF_P_RX_TMO  - receive timeout
 *          RF_P_RX_CRC_ERR  -received data crc error
 */
RF_P_RET_t Rf_SX1278_RxProcess( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    /* here, monitor RxDone Event, payload Crc Err Event, Receive Timeout Event, and Fhss event use irq beacuse it should be real time */
    
    /* if rx timeout event happened */
    if(1 == DIO1)
    {
        /* clear rx timeout irq flag */
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXTIMEOUT  );
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); /* goto standby mode */
        tRet = RF_P_RX_TMO;
    }
    /* if rx done interrupt */
    else if( DIO0 == 1 ) // RxDone
    {
        /* Clear rx done Irq flag */
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_RXDONE  );
        
#ifdef _SX1278DEBUG
        SX1276Read( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        if(( 1 == DIO3) || ((SX1276LR->RegHopChannel & 0x40) != 0x40))
        {
#else
        
            /* if payload crc event happened */
        if(1 == DIO3)
        {
#endif
            /* payload crc error irq and rx done irq will happen at the same time */
            /* clear payload crc error irq flag */
            SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_PAYLOADCRCERROR  );
            tRet = RF_P_RX_CRC_ERR;
        }
        else
        {
            /* Rx single mode */
            if( LoRaSettings.RxSingleOn == rf_true )
            {
                SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxBaseAddr;
                SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );

                if( LoRaSettings.ImplicitHeaderOn == rf_true )
                {
                    gs_u8PacketSize = SX1276LR->RegPayloadLength;
                    SX1276ReadFifo( gs_au8PacketBuffer, SX1276LR->RegPayloadLength );
                }
                else
                {             
                    SX1276Read( REG_LR_NBRXBYTES, &SX1276LR->RegNbRxBytes );
                    gs_u8PacketSize = SX1276LR->RegNbRxBytes;                    
                    SX1276ReadFifo( gs_au8PacketBuffer, gs_u8PacketSize );
                }
            }
            /* Rx continuous mode */
            else
            {
                SX1276Read( REG_LR_FIFORXCURRENTADDR, &SX1276LR->RegFifoRxCurrentAddr );

                if( LoRaSettings.ImplicitHeaderOn == rf_true )
                {
                    gs_u8PacketSize = SX1276LR->RegPayloadLength;
                    SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxCurrentAddr;
                    SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
                    SX1276ReadFifo( gs_au8PacketBuffer, SX1276LR->RegPayloadLength );
                }
                else
                {
                    SX1276Read( REG_LR_NBRXBYTES, &SX1276LR->RegNbRxBytes );
                    gs_u8PacketSize = SX1276LR->RegNbRxBytes;
                    SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoRxCurrentAddr;
                    SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
                    SX1276ReadFifo( gs_au8PacketBuffer, SX1276LR->RegNbRxBytes );
                }
            }
            tRet = RF_P_OK;
        }
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY ); /* goto standby mode */
        
    }
    
    /* others */
    else
    {
        tRet = RF_P_RUNNING;
    }
    
    return tRet;

}   /* Rf_SX1278_RxProcess() */

/***************************************************************************************************
 * @fn      Rf_RX1278_TxInit()
 *
 * @brief   Init Rf for transmit process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_RX1278_TxInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );

    if( LoRaSettings.FreqHopOn == rf_true )
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
        SX1276Read( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
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
    
    SX1276Write( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
    SX1276Write( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );
    
    SX1276LoRaSetPreambleLength(LoRaSettings.u16PreambleLength);    /* set preamble length */

    // Initializes the payload size
    SX1276LR->RegPayloadLength = gs_u8PacketSize;
    SX1276Write( REG_LR_PAYLOADLENGTH, SX1276LR->RegPayloadLength );

    if( LoRaSettings.RFFrequency > 860000000 )
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_RFO );
        SX1276LoRaSetPa20dBm( rf_false );
        LoRaSettings.Power = 14;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    }
    else
    {
        SX1276LoRaSetPAOutput( RFLR_PACONFIG_PASELECT_PABOOST );
        SX1276LoRaSetPa20dBm( rf_true );
        LoRaSettings.Power = 20;
        SX1276LoRaSetRFPower( LoRaSettings.Power );
    } 
    
    SX1276LR->RegFifoTxBaseAddr = 0x00; // Full buffer used for Tx
    SX1276Write( REG_LR_FIFOTXBASEADDR, SX1276LR->RegFifoTxBaseAddr );

    SX1276LR->RegFifoAddrPtr = SX1276LR->RegFifoTxBaseAddr;
    SX1276Write( REG_LR_FIFOADDRPTR, SX1276LR->RegFifoAddrPtr );
        
    // Write payload buffer to LORA modem
    SX1276WriteFifo( gs_au8PacketBuffer, gs_u8PacketSize );
        
                                    /* TxDone               RxTimeout                   FhssChangeChannel          ValidHeader         */
    SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_01 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_01;
                                    /* PllLock              Mode Ready  */
    SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_01 | RFLR_DIOMAPPING2_DIO5_00;
    SX1276WriteBuffer( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );
    
    sx1276_EnDio2Int(1);
    SX1276LoRaSetOpMode( RFLR_OPMODE_TRANSMITTER );

    /* clear Tx Time */
    gs_u32PacketTxTime = 0;
    
}   /* Rf_RX1278_TxInit() */


/***************************************************************************************************
 * @fn      Rf_SX1278_TxProcess()
 *
 * @brief   Rf radio transmit process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - transmit success
 *          RF_P_RUNNING  - receive process is running
 *          RF_P_TX_TMO  - receive timeout
 */
RF_P_RET_t Rf_SX1278_TxProcess( void )
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
            sx1276_EnDio2Int(0);
            /* Clear txdone Irq flag*/
            SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_TXDONE  );
            /* optimize the power consumption by switching off the transmitter as soon as the packet has been sent */
            SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
            tRet = RF_P_OK;
        }
    }
    return tRet;

}   /* Rf_SX1278_TxProcess */


/***************************************************************************************************
 * @fn      Rf_SX1278_CADInit()
 *
 * @brief   Init Rf for cad process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_SX1278_CADInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );

    /* set frequence hop options */
    if( LoRaSettings.FreqHopOn == rf_true )
    {
        SX1276LR->RegHopPeriod = LoRaSettings.HopPeriod;

        SX1276Read( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
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
    SX1276Write( REG_LR_IRQFLAGSMASK, SX1276LR->RegIrqFlagsMask );
    
                                // RxDone                   RxTimeout                   FhssChangeChannel           CadDone
    SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_10 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                // CAD Detected              ModeReady
    SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_01 | RFLR_DIOMAPPING2_DIO5_00;
    SX1276WriteBuffer( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );
        
    SX1276LoRaSetOpMode( RFLR_OPMODE_CAD );
    
    gs_u32PacketTxTime = 0;
    
}   /* Rf_SX1278_CADInit() */


/***************************************************************************************************
 * @fn      Rf_SX1278_CADProcess()
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
RF_P_RET_t Rf_SX1278_CADProcess( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    
    if(gs_u32PacketTxTime >= 5)
    {
        //tRet = RF_P_CAD_EMPTY;
    }
    
    if( DIO3 == 1 ) //CAD Done interrupt
    { 
        // Clear Irq
        SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDONE  );
        if( DIO1 == 1 ) // CAD Detected interrupt
        {
            // Clear Irq
            SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_CADDETECTED  );
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
}   /* Rf_SX1278_CADProcess() */

/***************************************************************************************************
 * @fn      Rf_SX1278_SleepInit()
 *
 * @brief   Init Rf for sleep process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_SX1278_SleepInit( void )
{
    SX1276LoRaSetOpMode( RFLR_OPMODE_SLEEP );
}   /* Rf_SX1278_SleepInit() */


/***************************************************************************************************
 * @fn      Rf_SX1278_SleepProcess()
 *
 * @brief   Rf radio sleep process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - transmit success
 *          RF_P_RUNNING  - receive process is running
 */
RF_P_RET_t Rf_SX1278_SleepProcess( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    
    sx1276_spi_deinit();
    sx1276_deinit();
    tRet = RF_P_OK;
    
    return tRet;
}   /* Rf_SX1278_SleepProcess() */


/***************************************************************************************************
 * @fn      SX1276FHSSChangeChannel_ISR()
 *
 * @brief   Rf radio Fhss change channel interrupt
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void SX1276FHSSChangeChannel_ISR(void)
{
    if( LoRaSettings.FreqHopOn == rf_true )
    {
        gs_u32PacketTxTime = 0;
        SX1276Read( REG_LR_HOPCHANNEL, &SX1276LR->RegHopChannel );
        SX1276LoRaSetRFFrequency( HOPPING_FREQUENCIES(SX1276LR->RegHopChannel & RFLR_HOPCHANNEL_CHANNEL_MASK));
        SX1276LoRaSetSymbTimeout(LoRaSettings.HopPeriod);
    }
    // Clear Irq
    SX1276Write( REG_LR_IRQFLAGS, RFLR_IRQFLAGS_FHSSCHANGEDCHANNEL );
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
void SX1276LoRaSetTxPacket( const void *a_pvdBuffer, rf_uint8 a_u8Size )
{
    gs_u8PacketSize = a_u8Size;
    memcpy( ( void * )gs_au8PacketBuffer, a_pvdBuffer, gs_u8PacketSize ); 
}   /* SX1276LoRaSetTxPacket() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetTxBufferAddress()
 *
 * @brief   set tx packet
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  txbuffer address
 */
void *SX1276LoRaGetTxBufferAddress(void)
{
    return ( void * ) gs_au8PacketBuffer;
}   /* SX1276LoRaGetTxBufferAddress() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetTxPacketSize()
 *
 * @brief   set tx packet size
 *
 * @author	chuanpengl
 *
 * @param   a_u8Size  - tx packet size
 *
 * @return  none
 */
void SX1276LoRaSetTxPacketSize( rf_uint8 a_u8Size )
{
    gs_u8PacketSize = a_u8Size;
}   /* SX1276LoRaGetTxPacketSize() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetRxPacket()
 *
 * @brief   get rx packet
 *
 * @author	chuanpengl
 *
 * @param   a_pvdBuffer  - data need to be send
 *          a_pu8Size  - data size
 *
 * @return  none
 */
void SX1276LoRaGetRxPacket( const void *a_pvdBuffer, rf_uint8 *a_pu8Size )
{
    *a_pu8Size = gs_u8PacketSize;
    memcpy((void *)a_pvdBuffer,  ( void * )gs_au8PacketBuffer, gs_u8PacketSize );
}   /* SX1276LoRaGetRxPacket() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetRxBufferAddress()
 *
 * @brief   get Rx packet buffer size
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rxbuffer address
 */
void *SX1276LoRaGetRxBufferAddress(void)
{
    return ( void * ) gs_au8PacketBuffer;
}   /* SX1276LoRaGetRxBufferAddress() */


/***************************************************************************************************
 * @fn      SX1276LoRaGetRxPacketSize()
 *
 * @brief   get rx packet size
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rx packet size
 */
rf_uint8 SX1276LoRaGetRxPacketSize( void )
{
    return gs_u8PacketSize;
}   /* SX1276LoRaGetRxPacketSize() */

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
void SX1276LoRaSetRFBaseFrequency( rf_uint32 a_u32Freq)
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
void SX1276LoRaSetRFFrequency( rf_uint32 a_u32Freq)
{
    a_u32Freq = ( rf_uint32 )( ( double )a_u32Freq / ( double )FREQ_STEP );
    
    SX1276LR->RegFrfMsb = ( rf_uint8 )( ( a_u32Freq >> 16 ) & 0xFF );
    SX1276LR->RegFrfMid = ( rf_uint8 )( ( a_u32Freq >> 8 ) & 0xFF );
    SX1276LR->RegFrfLsb = ( rf_uint8 )( a_u32Freq & 0xFF );

    SX1276WriteBuffer( REG_LR_FRFMSB, &SX1276LR->RegFrfMsb, 3 );
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
rf_uint32 SX1276LoRaGetRFFrequency( void )
{
    SX1276ReadBuffer( REG_LR_FRFMSB, &SX1276LR->RegFrfMsb, 3 );
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
void SX1276LoRaSetRFPower( rf_int8 a_i8Power )
{
    SX1276Read( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
    SX1276Read( REG_LR_PADAC, &SX1276LR->RegPaDac );
    
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
            SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( rf_uint8 )( ( uint16_t )( a_i8Power - 5 ) & 0x0F );
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
            SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( rf_uint8 )( ( uint16_t )( a_i8Power - 2 ) & 0x0F );
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
        SX1276LR->RegPaConfig = ( SX1276LR->RegPaConfig & RFLR_PACONFIG_OUTPUTPOWER_MASK ) | ( rf_uint8 )( ( uint16_t )( a_i8Power + 1 ) & 0x0F );
    }
    SX1276Write( REG_LR_PACONFIG, SX1276LR->RegPaConfig );
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
rf_int8 SX1276LoRaGetRFPower( void )
{
    SX1276Read( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
    SX1276Read( REG_LR_PADAC, &SX1276LR->RegPaDac );

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
void SX1276LoRaSetSignalBandwidth( rf_uint8 a_u8Bw )
{
    SX1276Read( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    SX1276LR->RegModemConfig1 = ( SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_BW_MASK ) | ( a_u8Bw << 4 );
    SX1276Write( REG_LR_MODEMCONFIG1, SX1276LR->RegModemConfig1 );
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
rf_uint8 SX1276LoRaGetSignalBandwidth( void )
{
    SX1276Read( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
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
void SX1276LoRaSetSpreadingFactor( rf_uint8 a_u8Factor )
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

    SX1276Read( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );    
    SX1276LR->RegModemConfig2 = ( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_SF_MASK ) | ( a_u8Factor << 4 );
    SX1276Write( REG_LR_MODEMCONFIG2, SX1276LR->RegModemConfig2 );    
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
rf_uint8 SX1276LoRaGetSpreadingFactor( void )
{
    SX1276Read( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );   
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
void SX1276LoRaSetErrorCoding( rf_uint8 a_u8Value )
{
    SX1276Read( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    SX1276LR->RegModemConfig1 = ( SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_CODINGRATE_MASK ) | ( a_u8Value << 1 );
    SX1276Write( REG_LR_MODEMCONFIG1, SX1276LR->RegModemConfig1 );
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
rf_uint8 SX1276LoRaGetErrorCoding( void )
{
    SX1276Read( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
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
 * @param   a_bEnable  - rf_true = enable crc, rf_false = disable
 *
 * @return  none
 */
void SX1276LoRaSetPacketCrcOn( rf_bool a_bEnable )
{
    SX1276Read( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );
    SX1276LR->RegModemConfig2 = ( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_MASK ) | ( a_bEnable << 2 );
    SX1276Write( REG_LR_MODEMCONFIG2, SX1276LR->RegModemConfig2 );
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
 * @return  rf_true  - crc is on
 *          rf_false  - crc is off
 */
rf_bool SX1276LoRaGetPacketCrcOn( void )
{
    SX1276Read( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2 );
    LoRaSettings.CrcOn = (( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_RXPAYLOADCRC_ON ) >> 1) > 0 ? rf_true : rf_false;
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
    SX1276ReadBuffer( REG_LR_PREAMBLEMSB, &SX1276LR->RegPreambleMsb, 2 );

    SX1276LR->RegPreambleMsb = ( a_u16Value >> 8 ) & 0x00FF;
    SX1276LR->RegPreambleLsb = a_u16Value & 0xFF;
    SX1276WriteBuffer( REG_LR_PREAMBLEMSB, &SX1276LR->RegPreambleMsb, 2 );
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
    SX1276ReadBuffer( REG_LR_PREAMBLEMSB, &SX1276LR->RegPreambleMsb, 2 );
    return ( ( SX1276LR->RegPreambleMsb & 0x00FF ) << 8 ) | SX1276LR->RegPreambleLsb;
}   /* SX1276LoRaGetPreambleLength() */


/***************************************************************************************************
 * @fn      SX1276LoRaSetImplicitHeaderOn()
 *
 * @brief   enable implicit header or not
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rf_true = enable implicit header, rf_false = disable implicit header
 *
 * @return  none
 */
void SX1276LoRaSetImplicitHeaderOn( rf_bool a_bEnable )
{
    SX1276Read( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    SX1276LR->RegModemConfig1 = ( SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_MASK ) | ( a_bEnable );
    SX1276Write( REG_LR_MODEMCONFIG1, SX1276LR->RegModemConfig1 );
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
 * @return  rf_true  - implicit header is on
 *          rf_false  - implicit header is off
 */
rf_bool SX1276LoRaGetImplicitHeaderOn( void )
{
    SX1276Read( REG_LR_MODEMCONFIG1, &SX1276LR->RegModemConfig1 );
    LoRaSettings.ImplicitHeaderOn = ((SX1276LR->RegModemConfig1 & RFLR_MODEMCONFIG1_IMPLICITHEADER_ON)) > 0 ?\
      rf_true : rf_false;
    return LoRaSettings.ImplicitHeaderOn;
}   /* SX1276LoRaGetImplicitHeaderOn */


/***************************************************************************************************
 * @fn      SX1276LoRaSetRxSingleOn()
 *
 * @brief   enable rx single mode 
 *
 * @author	chuanpengl
 *
 * @param   a_bEnable  - rf_true = rx single on, rf_false = rx continuous mode 
 *
 * @return  none
 */
void SX1276LoRaSetRxSingleOn( rf_bool a_bEnable )
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
 * @return  rf_true  - rx single on
 *          rf_false  - rx continuous
 */
rf_bool SX1276LoRaGetRxSingleOn( void )
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
 * @param   a_bEnable  - rf_true = enable, rf_false = disable 
 *
 * @return  none
 */
void SX1276LoRaSetFreqHopOn( rf_bool a_bEnable )
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
 * @return  rf_true  - hop is enable
 *          rf_false  - hop is disable
 */
rf_bool SX1276LoRaGetFreqHopOn( void )
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
void SX1276LoRaSetHopPeriod( rf_uint8 a_u8Value )
{
    SX1276LR->RegHopPeriod = a_u8Value;
    SX1276Write( REG_LR_HOPPERIOD, SX1276LR->RegHopPeriod );
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
rf_uint8 SX1276LoRaGetHopPeriod( void )
{
    SX1276Read( REG_LR_HOPPERIOD, &SX1276LR->RegHopPeriod );
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
void SX1276LoRaSetPayloadLength( rf_uint8 a_u8Value )
{
    SX1276LR->RegPayloadLength = a_u8Value;
    SX1276Write( REG_LR_PAYLOADLENGTH, SX1276LR->RegPayloadLength );
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
rf_uint8 SX1276LoRaGetPayloadLength( void )
{
    SX1276Read( REG_LR_PAYLOADLENGTH, &SX1276LR->RegPayloadLength );
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
 * @param   rf_true  - enable +20dBm output
 *          rf_false  - disable +20dBm output
 *
 * @return  none
 */
void SX1276LoRaSetPa20dBm( rf_bool enale )
{
    SX1276Read( REG_LR_PADAC, &SX1276LR->RegPaDac );
    SX1276Read( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );

    if( ( SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_PABOOST ) == RFLR_PACONFIG_PASELECT_PABOOST )
    {    
        if( enale == rf_true )
        {
            SX1276LR->RegPaDac = 0x87;
        }
    }
    else
    {
        SX1276LR->RegPaDac = 0x84;
    }
    SX1276Write( REG_LR_PADAC, SX1276LR->RegPaDac );
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
 * @return  rf_true  - +20dBm output is enable
 *          rf_false  - +20dBm output is disable
 */
rf_bool SX1276LoRaGetPa20dBm( void )
{
    SX1276Read( REG_LR_PADAC, &SX1276LR->RegPaDac );
    
    return ( ( SX1276LR->RegPaDac & 0x07 ) == 0x07 ) ? rf_true : rf_false;
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
void SX1276LoRaSetPAOutput( rf_uint8 a_u8OutputPin )
{
    SX1276Read( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
    SX1276LR->RegPaConfig = (SX1276LR->RegPaConfig & RFLR_PACONFIG_PASELECT_MASK ) | a_u8OutputPin;
    SX1276Write( REG_LR_PACONFIG, SX1276LR->RegPaConfig );
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
rf_uint8 SX1276LoRaGetPAOutput( void )
{
    SX1276Read( REG_LR_PACONFIG, &SX1276LR->RegPaConfig );
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
void SX1276LoRaSetPaRamp( rf_uint8 a_u8Value )
{
    SX1276Read( REG_LR_PARAMP, &SX1276LR->RegPaRamp );
    SX1276LR->RegPaRamp = ( SX1276LR->RegPaRamp & RFLR_PARAMP_MASK ) | ( a_u8Value & ~RFLR_PARAMP_MASK );
    SX1276Write( REG_LR_PARAMP, SX1276LR->RegPaRamp );
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
rf_uint8 SX1276LoRaGetPaRamp( void )
{
    SX1276Read( REG_LR_PARAMP, &SX1276LR->RegPaRamp );
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
void SX1276LoRaSetSymbTimeout( rf_uint16 a_u16Value )
{
    SX1276ReadBuffer( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2, 2 );

    SX1276LR->RegModemConfig2 = ( SX1276LR->RegModemConfig2 & RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK ) | ( ( a_u16Value >> 8 ) & ~RFLR_MODEMCONFIG2_SYMBTIMEOUTMSB_MASK );
    SX1276LR->RegSymbTimeoutLsb = a_u16Value & 0xFF;
    SX1276WriteBuffer( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2, 2 );
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
rf_uint16 SX1276LoRaGetSymbTimeout( void )
{
    SX1276ReadBuffer( REG_LR_MODEMCONFIG2, &SX1276LR->RegModemConfig2, 2 );
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
void SX1276LoRaSetLowDatarateOptimize( rf_bool a_bEnable )
{
    SX1276Read( REG_LR_MODEMCONFIG3, &SX1276LR->RegModemConfig3 );
    SX1276LR->RegModemConfig3 = ( SX1276LR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_MASK ) | ( a_bEnable << 3 );
    SX1276Write( REG_LR_MODEMCONFIG3, SX1276LR->RegModemConfig3 );
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
rf_bool SX1276LoRaGetLowDatarateOptimize( void )
{
    SX1276Read( REG_LR_MODEMCONFIG3, &SX1276LR->RegModemConfig3 );
    return (( SX1276LR->RegModemConfig3 & RFLR_MODEMCONFIG3_LOWDATARATEOPTIMIZE_ON ) >> 3 ) > 0 ? rf_true : rf_false;
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
void SX1276LoRaSetNbTrigPeaks( rf_uint8 a_u8Value )
{
    SX1276Read( 0x31, &SX1276LR->RegTestReserved31 );
    SX1276LR->RegTestReserved31 = ( SX1276LR->RegTestReserved31 & 0xF8 ) | a_u8Value;
    SX1276Write( 0x31, SX1276LR->RegTestReserved31 );
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
static rf_uint8 SX1276LoRaGetNbTrigPeaks( void )
{
    SX1276Read( 0x31, &SX1276LR->RegTestReserved31 );
    return ( SX1276LR->RegTestReserved31 & 0x07 );
}/* SX1276LoRaGetNbTrigPeaks() */
 

/***************************************************************************************************
 * @fn      SX1276LoRaSetOpMode()
 *
 * @brief   set lora operation mode 
 *
 * @author	chuanpengl
 *
 * @param   a_u8OpMode  - operation mode
 *
 * @return  none
 */
void SX1276LoRaSetOpMode( rf_uint8 a_u8OpMode )
{
    if( a_u8OpMode == RFLR_OPMODE_TRANSMITTER )
    {
        RXTX( rf_true );
    }
    else
    {
        RXTX( rf_false );
    }

    SX1276LR->RegOpMode = ( SX1276LR->RegOpMode & RFLR_OPMODE_MASK ) | a_u8OpMode;
    SX1276Write( REG_LR_OPMODE, SX1276LR->RegOpMode );
}


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
void SX1276Reset( void )
{
    uint32_t startTick;
    SX1276SetReset( RADIO_RESET_ON );
    
    // Wait 1ms
    startTick = GET_TICK_COUNT( );
    while( ( GET_TICK_COUNT( ) - startTick ) < TICK_RATE_MS( 1 ) );    

    SX1276SetReset( RADIO_RESET_OFF );
    
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
 * @param   a_bEnable  - rf_true = enable lora, rf_false  - disable lora
 *
 * @return  none
 */
void SX1276SetLoRaOn( rf_bool a_bEnable )
{
    if( a_bEnable == rf_true )
    {
        SX1276LoRaSetOpMode( RFLR_OPMODE_SLEEP );
        
        SX1276LR->RegOpMode = ( SX1276LR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_ON;
        SX1276Write( REG_LR_OPMODE, SX1276LR->RegOpMode );
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
                                        // RxDone               RxTimeout                   FhssChangeChannel           CadDone
        SX1276LR->RegDioMapping1 = RFLR_DIOMAPPING1_DIO0_00 | RFLR_DIOMAPPING1_DIO1_00 | RFLR_DIOMAPPING1_DIO2_00 | RFLR_DIOMAPPING1_DIO3_00;
                                        // CadDetected          ModeReady
        SX1276LR->RegDioMapping2 = RFLR_DIOMAPPING2_DIO4_00 | RFLR_DIOMAPPING2_DIO5_00;
        SX1276WriteBuffer( REG_LR_DIOMAPPING1, &SX1276LR->RegDioMapping1, 2 );

        SX1276ReadBuffer( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );
    }
    else
    {
        SX1276LoRaSetOpMode( RFLR_OPMODE_SLEEP );
        
        SX1276LR->RegOpMode = ( SX1276LR->RegOpMode & RFLR_OPMODE_LONGRANGEMODE_MASK ) | RFLR_OPMODE_LONGRANGEMODE_OFF;
        SX1276Write( REG_LR_OPMODE, SX1276LR->RegOpMode );
        
        SX1276LoRaSetOpMode( RFLR_OPMODE_STANDBY );
        
        SX1276ReadBuffer( REG_LR_OPMODE, SX1276Regs + 1, 0x70 - 1 );
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
void Rf_Sx1276_SetPreambleLengthPara(rf_uint16 a_u16length)
{
    LoRaSettings.u16PreambleLength = a_u16length;
}   /* Rf_Sx1276_SetPreambleLengthPara() */


/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/