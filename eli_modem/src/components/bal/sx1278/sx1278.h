/***************************************************************************************************
 * Copyright (c) 2015-2020 Intelligent Network System Ltd. All Rights Reserved. 
 * 
 * This software is the confidential and proprietary information of Founder. You shall not disclose
 * such Confidential Information and shall use it only in accordance with the terms of the 
 * agreements you entered into with Founder. 
***************************************************************************************************/
/***************************************************************************************************
* @file name    sx1278.h
* @data         2015/07/07
* @auther       chuanpengl
* @module       sx1278 domain
* @brief        sx1278
***************************************************************************************************/
#ifndef __SX1278_H__
#define __SX1278_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "sx1278_cfg.h"

#if (BAL_USE_SX1278 == BAL_MODULE_ON)

#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */


/***************************************************************************************************
 * TYPEDEFS
 */
/*!
 * SX1276 LoRa General parameters definition
 */
typedef struct sLoRaSettings
{
    rfUint32 RFFrequency;
    rfInt8 Power;
    rfUint8 SignalBw;                   // LORA [0: 7.8 kHz, 1: 10.4 kHz, 2: 15.6 kHz, 3: 20.8 kHz, 4: 31.2 kHz,
                                        // 5: 41.6 kHz, 6: 62.5 kHz, 7: 125 kHz, 8: 250 kHz, 9: 500 kHz, other: Reserved]  
    rfUint8 SpreadingFactor;            // LORA [6: 64, 7: 128, 8: 256, 9: 512, 10: 1024, 11: 2048, 12: 4096  chips]
    rfUint8 ErrorCoding;                // LORA [1: 4/5, 2: 4/6, 3: 4/7, 4: 4/8]
    rfBool CrcOn;                         // [0: OFF, 1: ON]
    rfBool ImplicitHeaderOn;              // [0: OFF, 1: ON]
    rfBool RxSingleOn;                    // [0: Continuous, 1 Single]
    rfBool FreqHopOn;                     // [0: OFF, 1: ON]
    rfUint8 HopPeriod;                  // Hops every frequency hopping period symbols
    rfUint32 TxPacketTimeout;
    rfUint32 RxPacketTimeout;
    rfUint8 PayloadLength;
    rfUint16 u16PreambleLength;
}tLoRaSettings;


/*!
 * RF packet definition
 */
#define RF_BUFFER_SIZE_MAX                          256
#define RF_BUFFER_SIZE                              256

/*!
 * RF state machine
 */
//LoRa
typedef enum
{
    RFLR_STATE_IDLE,
    RFLR_STATE_RX_INIT,
    RFLR_STATE_RX_RUNNING,
    RFLR_STATE_RX_DONE,
    RFLR_STATE_RX_TIMEOUT,
    RFLR_STATE_TX_INIT,
    RFLR_STATE_TX_RUNNING,
    RFLR_STATE_TX_DONE,
    RFLR_STATE_TX_TIMEOUT,
    RFLR_STATE_CAD_INIT,
    RFLR_STATE_CAD_RUNNING,
}tRFLRStates;

typedef enum
{
    RF_P_OK = 0,            /* ret ok */
    RF_P_RUNNING = 1,      /* it is running */
    RF_P_RX_TMO = 2,        /* timeout error */    
    RF_P_RX_CRC_ERR = 3,    /* receive data crc error */
    RF_P_TX_TMO = 4,        /* transmit data timeout */
    RF_P_CFG_INVALID_PARA = 5,  /* config parameter is invalid */
    RF_P_CFG_TIMEOUT = 6,    /* config alive time timeout */
    RF_P_CAD_EMPTY = 7,          /* cad channel is empty */
    RF_P_UNKOWN_STANDBY = 8
    
}RF_P_RET_t;


#define _IMPORT  extern
_IMPORT unsigned long systick_Get(void);
#define GET_TICK_COUNT()            systick_Get()
/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * GLOBAL FUNCTIONS DECLEAR
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
void sx1278_Init( void );

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
void sx1278_WakeUpInit( void );

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
void sx1278_Stop( void );

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
void sx1278_RxInit( void );

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
RF_P_RET_t sx1278_RxProcess( void );

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
void sx1278_TxInit( void );

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
RF_P_RET_t sx1278_TxProcess( void );

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
void sx1278_CADInit( void );

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
RF_P_RET_t sx1278_CADProcess( void );

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
void sx1278_SleepInit( void );

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
RF_P_RET_t sx1278_SleepProcess( void );



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
void *sx1278_GetBufferAddr(void);


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
void sx1278_SetPacketSize( rfUint16 a_u16Size );


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
rfUint16 sx1278_GetPacketSize( void );



/***************************************************************************************************
 * @fn      SX1276LoRaSetRFBaseFrequency()
 *
 * @brief   set LoRa base frequence, it will be the first frequence when hop
 *
 * @author  chuanpengl
 *
 * @param   a_u32Freq  - rf frequence, unit is Hz
 *
 * @return  none
 */
extern void SX1276LoRaSetRFBaseFrequency( rfUint32 a_u32Freq);


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
extern void SX1276LoRaSetRFFrequency( rfUint32 a_u32Freq);


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
extern rfUint32 SX1276LoRaGetRFFrequency( void );


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
extern void SX1276LoRaSetRFPower( rfInt8 a_i8Power );


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
extern rfInt8 SX1276LoRaGetRFPower( void );


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
extern void SX1276LoRaSetSignalBandwidth( rfUint8 a_u8Bw );


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
extern rfUint8 SX1276LoRaGetSignalBandwidth( void );


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
extern void SX1276LoRaSetSpreadingFactor( rfUint8 a_u8Factor );


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
extern rfUint8 SX1276LoRaGetSpreadingFactor( void );


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
extern void SX1276LoRaSetErrorCoding( rfUint8 a_u8Value );


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
extern rfUint8 SX1276LoRaGetErrorCoding( void );


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
extern void SX1276LoRaSetPacketCrcOn( rfBool a_bEnable );


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
extern rfBool SX1276LoRaGetPacketCrcOn( void );


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
extern void SX1276LoRaSetPreambleLength( rfUint16 a_u16Value );


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
extern rfUint16 SX1276LoRaGetPreambleLength( void );


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
extern void SX1276LoRaSetImplicitHeaderOn( rfBool a_bEnable );

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
extern rfBool SX1276LoRaGetImplicitHeaderOn( void );


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
extern void SX1276LoRaSetRxSingleOn( rfBool a_bEnable );


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
extern rfBool SX1276LoRaGetRxSingleOn( void );


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
extern void SX1276LoRaSetFreqHopOn( rfBool a_bEnable );


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
extern rfBool SX1276LoRaGetFreqHopOn( void );


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
extern void SX1276LoRaSetHopPeriod( rfUint8 a_u8Value );


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
extern rfUint8 SX1276LoRaGetHopPeriod( void );


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
extern void SX1276LoRaSetTxPacketTimeout( rfUint32 value );


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
extern rfUint32 SX1276LoRaGetTxPacketTimeout( void );


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
extern void SX1276LoRaSetRxPacketTimeout( rfUint32 a_u32Value );


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
extern rfUint32 SX1276LoRaGetRxPacketTimeout( void );


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
extern void SX1276LoRaSetPayloadLength( rfUint8 a_u8Value );


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
extern rfUint8 SX1276LoRaGetPayloadLength( void );


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
extern void SX1276LoRaSetPa20dBm( rfBool enale );


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
extern rfBool SX1276LoRaGetPa20dBm( void );


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
extern void SX1276LoRaSetPAOutput( rfUint8 a_u8OutputPin );


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
extern rfUint8 SX1276LoRaGetPAOutput( void );


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
extern void SX1276LoRaSetPaRamp( rfUint8 a_u8Value );


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
extern rfUint8 SX1276LoRaGetPaRamp( void );


/***************************************************************************************************
 * @fn      SX1276LoRaSetSymbTimeout()
 *
 * @brief   set symbol time
 *
 * @author	author name
 *
 * @param   a_u16Value  - symbol time 
 *
 * @return  none
 */
extern void SX1276LoRaSetSymbTimeout( rfUint16 a_u16Value );


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
extern rfUint16 SX1276LoRaGetSymbTimeout( void );
 
 
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
extern void SX1276LoRaSetLowDatarateOptimize( rfBool a_bEnable );
 
 
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
extern rfBool SX1276LoRaGetLowDatarateOptimize( void );


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
extern rfBool SX1276LoRaGetLowDatarateOptimize( void );


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
extern void SX1276LoRaSetNbTrigPeaks( rfUint8 a_u8Value );

/***************************************************************************************************
 * @fn      SX1276LoRaSetOpMode()
 *
 * @brief   set lora operation mode 
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  return get value
 */
extern void SX1276LoRaSetOpMode( rfUint8 a_u8OpMode );

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
extern rfUint8 SX1278LoRaGetOpMode( void );


rfInt8 SX1276LoRaGetPacketSnr( void );

double SX1276LoRaGetPacketRssi( void );

#ifdef __cplusplus
}
#endif

#endif /* (BAL_USE_SX1278 == BAL_MODULE_ON) */

#endif /* __SX1278_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by chuanpengl @ 20150707
*   context: here write modified history
*
***************************************************************************************************/
