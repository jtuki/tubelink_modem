/***************************************************************************************************
* @file name	rf_config.h
* @data   		2014/11/14
* @auther   	chuanpengl
* @module   	rf config
* @brief 		rf radio config
***************************************************************************************************/
#ifndef __RF_CONFIG_H__
#define __RF_CONFIG_H__
/***************************************************************************************************
 * INCLUDES
 */
#include "rf_type.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

/* rf radio type select */
#define RF_SX1278   (0x0001)
#define RF_CC1120   (0x0002)

#define RF_SELECT   RF_SX1278

/* api config macros */
#if RF_SELECT == RF_SX1278
#define Rf_P_Init                   Rf_SX1278_Init
#define Rf_P_WakeUpInit             Rf_SX1278_WakeUpInit
#define Rf_P_Stop                   Rf_SX1278_Stop
#define Rf_P_RxInit                 Rf_SX1278_RxInit
#define Rf_P_RxProcess              Rf_SX1278_RxProcess
#define Rf_P_TxInit                 Rf_RX1278_TxInit
#define Rf_P_TxProcess              Rf_SX1278_TxProcess
#define Rf_P_CADInit                Rf_SX1278_CADInit
#define Rf_P_CADProcess             Rf_SX1278_CADProcess
#define Rf_P_SleepInit              Rf_SX1278_SleepInit
#define Rf_P_SleepProcess           Rf_SX1278_SleepProcess
#define Rf_P_DIO2Isr                SX1276FHSSChangeChannel_ISR
#define Rf_P_SetTxPacket            SX1276LoRaSetTxPacket
#define Rf_P_GetRxPacket            SX1276LoRaGetRxPacket
#define Rf_P_GetTxBufferAddress     SX1276LoRaGetTxBufferAddress
#define Rf_P_SetTxPacketSize        SX1276LoRaSetTxPacketSize
#define Rf_P_GetRxBufferAddress     SX1276LoRaGetRxBufferAddress
#define Rf_P_GetRxPacketSize        SX1276LoRaGetRxPacketSize
#define Rf_P_UpdateTxPacketTime     Rf_Sx1276_UpdateTxPacketTime
#define Rf_P_SetPreambleLengthPara  Rf_Sx1276_SetPreambleLengthPara
#define Rf_P_SetRFPower             SX1276LoRaSetRFPower

#define Rf_P_GetPacketSnr           SX1276LoRaGetPacketSnr
#define Rf_P_GetPacketRssi         SX1276LoRaGetPacketRssi

#define Rf_Cfg_Init                 Sx1276_Cfg_Init
#define Rf_Cfg_Process              Sx1276_Cfg_Process
#define Rf_Cfg_GetBufferAddr        Sx1276_Cfg_GetBufferAddr
#define Rf_Cfg_SetBufferSize        Sx1276_Cfg_SetBufferSize
#define Rf_Cfg_GetBufferSize        Sx1276_Cfg_GetBufferSize
#define Rf_Cfg_UpdateAliveTime      Sx1276_Cfg_UpdateAliveTime
#define Rf_Cfg_ExcuteParameters     Sx1276_Cfg_ExcuteParameters
#define Rf_Cfg_GetUartBaud          Sx1276_Cfg_GetUartBaud
#define Rf_Cfg_GetUartParity        Sx1276_Cfg_GetUartParity
#define Rf_Cfg_GetWakeupTime        Sx1276_Cfg_GetWakeupTime
#define Rf_Cfg_GetPreamble          Sx1276_Cfg_GetPreamble
#elif RF_SELECT == RF_CC1120
#define Rf_P_Init           Rf_CC1120_Init



#endif
/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */
typedef enum
{
    RF_P_OK = 0,            /* ret ok */
    RF_P_RUNNING = 1,      /* it is running */
    RF_P_RX_TMO = 2,        /* timeout error */    
    RF_P_RX_CRC_ERR = 3,    /* receive data crc error */
    RF_P_TX_TMO = 4,        /* transmit data timeout */
    RF_P_CFG_INVALID_PARA = 5,  /* config parameter is invalid */
    RF_P_CFG_TIMEOUT = 6,    /* config alive time timeout */
    RF_P_CAD_EMPTY = 7          /* cad channel is empty */
}RF_P_RET_t;

/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * EXTERNAL FUNCTIONS DECLEAR
 */

 


/***************************************************************************************************
 * @fn      Rf_P_Init()
 *
 * @brief   Rf radio init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_Init( void );

     
/***************************************************************************************************
 * @fn      Rf_P_WakeUpInit()
 *
 * @brief   Rf radio wakeup init, quick init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_WakeUpInit( void );


/***************************************************************************************************
 * @fn      Rf_P_Stop()
 *
 * @brief   stop Rf, rf goto standby state
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_Stop( void );


/***************************************************************************************************
 * @fn      Rf_P_RxInit()
 *
 * @brief   Init Rf for receive process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_RxInit( void );


/***************************************************************************************************
 * @fn      Rf_P_RxProcess()
 *
 * @brief   Rf receive process
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
extern RF_P_RET_t Rf_P_RxProcess( void );


/***************************************************************************************************
 * @fn      Rf_P_TxInit()
 *
 * @brief   Init Rf for transmit process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_TxInit( void );


/***************************************************************************************************
 * @fn      Rf_P_TxProcess()
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
extern RF_P_RET_t Rf_P_TxProcess( void );


/***************************************************************************************************
 * @fn      Rf_P_CadInit()
 *
 * @brief   Init Rf for cad mode
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_CadInit( void );


/***************************************************************************************************
 * @fn      Rf_P_CadProcess()
 *
 * @brief   Rf radio cad process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - cad success
 */
extern RF_P_RET_t Rf_P_CadProcess( void );


/***************************************************************************************************
 * @fn      Rf_P_CADInit()
 *
 * @brief   Init Rf for cad process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_CADInit( void );


/***************************************************************************************************
 * @fn      Rf_P_CADProcess()
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
extern RF_P_RET_t Rf_P_CADProcess( void );


/***************************************************************************************************
 * @fn      Rf_P_SleepInit()
 *
 * @brief   Init Rf for sleep process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_SleepInit( void );


/***************************************************************************************************
 * @fn      Rf_P_SleepProcess()
 *
 * @brief   Rf radio sleep process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - sleep enter success
 */
extern RF_P_RET_t Rf_P_SleepProcess( void );


/***************************************************************************************************
 * @fn      Rf_P_ConfigInit()
 *
 * @brief   Init Rf radio for config process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_ConfigInit( void );


/***************************************************************************************************
 * @fn      Rf_P_ConfigProcess()
 *
 * @brief   Rf radio config process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - config success
 */
extern RF_P_RET_t Rf_P_ConfigProcess( void );


/***************************************************************************************************
 * @fn      Rf_P_DIO2Isr()
 *
 * @brief   dio 2 isr
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_DIO2Isr( void );


/***************************************************************************************************
 * @fn      Rf_P_SetTxPacket()
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
extern void Rf_P_SetTxPacket( const void *a_pvdBuffer, rf_uint8 a_u8Size );

 
/***************************************************************************************************
 * @fn      Rf_P_GetRxPacket()
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
extern void Rf_P_GetRxPacket( const void *a_pvdBuffer, rf_uint8 *a_pu8Size );
     

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
extern void *Rf_P_GetTxBufferAddress(void);


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
extern void Rf_P_SetTxPacketSize( rf_uint8 a_u8Size );


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
extern void *Rf_P_GetRxBufferAddress(void);


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
extern rf_uint8 Rf_P_GetRxPacketSize( void );


/***************************************************************************************************
 * @fn      Rf_P_UpdateTxPacketTime()
 *
 * @brief   update tx time, called every milisecond
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_P_UpdateTxPacketTime(void);


/***************************************************************************************************
 * @fn      Rf_P_SetPreambleLengthPara()
 *
 * @brief   set preamble length for LoRaSettings parameter
 *
 * @author	chuanpengl
 *
 * @param   a_u16Length  - paramble length
 *
 * @return  none
 */
extern void Rf_P_SetPreambleLengthPara(rf_uint16 a_u16length);

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
void SX1276LoRaSetRFPower( rf_int8 a_i8Power );

extern rf_int8 Rf_P_GetPacketSnr( void );

extern double Rf_P_GetPacketRssi( void );

/***************************************************************************************************
 * @fn      Rf_Cfg_Init()
 *
 * @brief   do rf init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_Cfg_Init( void );


/***************************************************************************************************
 * @fn      Rf_Cfg_Process()
 *
 * @brief   do rf process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  RF_P_OK  - config ok
 *          RF_RUNNING  - config in process
 *          RF_P_CFG_INVALID_PARA  - invalid parameter
 *          RF_P_CFG_TIMEOUT  - config timeout
 */
extern RF_P_RET_t Rf_Cfg_Process( void );


/***************************************************************************************************
 * @fn      Rf_Cfg_GetBufferAddr()
 *
 * @brief   get config buffer Address
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  buffer address
 */
extern void* Rf_Cfg_GetBufferAddr( void );


/***************************************************************************************************
 * @fn      Rf_Cfg_SetBufferSize()
 *
 * @brief   set config buffer size
 *
 * @author	chuanpengl
 *
 * @param   a_u8Size  - config buffer size
 *
 * @return  none
 */
extern void Rf_Cfg_SetBufferSize( rf_uint8 a_u8Size );


/***************************************************************************************************
 * @fn      Rf_Cfg_GetBufferSize()
 *
 * @brief   set config buffer size
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  config buffer size
 */
extern rf_uint8 Rf_Cfg_GetBufferSize( void );


/***************************************************************************************************
 * @fn      Rf_Cfg_UpdateAliveTime()
 *
 * @brief   update config alive time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_Cfg_UpdateAliveTime( void );


/***************************************************************************************************
 * @fn      Rf_Cfg_ExcuteParameters()
 *
 * @brief   excute parameter
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_Cfg_ExcuteParameters( void );


/***************************************************************************************************
 * @fn      Rf_Cfg_GetUartBaud()
 *
 * @brief   get config uart baud
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  uart baud
 */
extern rf_uint32 Rf_Cfg_GetUartBaud( void );

/***************************************************************************************************
 * @fn      Rf_Cfg_GetUartParity()
 *
 * @brief   get config uart parity
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  uart parity
 */
extern rf_uint8 Rf_Cfg_GetUartParity( void );

/***************************************************************************************************
 * @fn      Rf_Cfg_GetWakeupTime()
 *
 * @brief   get config wakeup time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rf_uint16 Rf_Cfg_GetWakeupTime( void );


/***************************************************************************************************
 * @fn      Rf_Cfg_GetPreamble()
 *
 * @brief   get config get preamble
 *
 * @author	chuanpengl
 *
 * @param   a_bWakeupMode  - rf_true = wakeup mode
 *                         - rf_false = normal mode
 *
 * @return  none
 */
extern rf_uint16 Rf_Cfg_GetPreamble(rf_bool a_bWakeupMode);


#endif /* __RF_CONFIG_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/