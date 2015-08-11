/***************************************************************************************************
* @file name    rf_manager.h
* @data         2014/11/14
* @auther       chuanpengl
* @module       rf manager
* @brief        provide apis of rf module
***************************************************************************************************/
#ifndef __RF_MANAGER_H__
#define __RF_MANAGER_H__

/***************************************************************************************************
 * INCLUDES
 */
#include "sx1278_cfg.h"

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

/* rf state define  */
typedef enum rf_state
{
    RF_STANDBY = 0,         /* rf stand by state */
    RF_RX = 1,              /* rf receive state */
    RF_TX = 2,              /* rf transmit state */
    RF_SLEEP = 3,           /* rf sleep state */
    RF_CAD = 4,             /* rf cad(channel activity detection) state */
    RF_CONFIG = 5           /* rf config state */
}RF_STATE_t;


/* rf return code */
typedef enum
{
    RF_RET_OK = 0,
    RF_RET_TX_MSG_LONG = 1,
    RF_RET_TX_BUSY = 2,
    RF_RET_RX_BUF_SMALL = 3,
    RF_RET_RX_NOT_FINISHED = 4,
    RF_RET_PARA_ERROR = 5
}RF_RET_t;

typedef enum
{
    RF_RTE_INIT = 0,
    RF_RTE_RUN = 1,
    RF_RTE_SLEEP = 2,
    RF_RTE_CFG_SUCCESS = 3,
    RF_RTE_CFG_FAILED = 4,
    RF_RTE_CFG_EXIT = 5,
    RF_RTE_CAD_DETECTED = 6,
    RF_RTE_CAD_EMPTY = 7,
    RF_RTE_RX_END = 8,
    RF_RTE_STANDBY = 9
}RF_RTE_RET_t;

typedef enum
{
    RF_EVT_IDLE = 0,
    RF_EVT_RX_OK,
    RF_EVT_RX_TMO,
    RF_EVT_RX_CRCERR,
    RF_EVT_TX_OK,
    RF_EVT_TX_TMO
}RF_EVT_e;

typedef void (*Rf_Event)(RF_EVT_e a_tEvt);

/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * EXTERNAL FUNCTIONS DECLEAR
 */

/***************************************************************************************************
 * @fn      Rf_Init()
 *
 * @brief   RF Init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_Init( Rf_Event a_hEvt );


/***************************************************************************************************
 * @fn      Rf_WakeUpInit()
 *
 * @brief   RF Wakeup quick Init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_WakeUpInit( void );


/***************************************************************************************************
 * @fn      Rf_Routine()
 *
 * @brief   RF Routine
 *
 * @author	chuanpengl
 *
 * @param   a_pvdArg  - no use now, only for extend
 *
 * @return  RF_RTE_SLEEP  - rf goto sleep
 *          RF_RTE_INIT  - rf need init
 *          RF_RTE_RUN  - rf is run
 */
extern RF_RTE_RET_t Rf_Routine(void *a_pvdArg);


/***************************************************************************************************
 * @fn      Rf_Send()
 *
 * @brief   RF send message request(request may be failed)
 *
 * @author	chuanpengl
 *
 * @param   a_pcData  - data need to be send through rf radio
 *          a_u16Len  - data length
 *
 * @return  RF_RET_OK  - request success, request data will be send later
 *          RF_RET_TX_MSG_LONG  - request failed, message is too long, rf radio can't send it
 *          RF_RET_TX_BUSY  - request failed, rf tx busy, can not receive request
 */
extern RF_RET_t Rf_Send( rfChar *a_pcData, rfUint16 a_u16Len );


 /***************************************************************************************************
 * @fn      Rf_Get()
 *
 * @brief   RF get receive message, if receive process is not finished, it will return failed
 *
 * @author	chuanpengl
 *
 * @param   a_pcData  - buffer used for get receive data, the first byte is receive length, from 
 *                      byte 1, are receive data
 *          a_u16Len  - buffer size
 *
 * @return  receive length
 */
rfUint16 Rf_Get( rfChar *a_pcData, rfUint16 a_u16Len );

 /***************************************************************************************************
 * @fn      Rf_ReceiveStart()
 *
 * @brief   start receive operation
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_ReceiveStart( void );

 /***************************************************************************************************
 * @fn      Rf_Stop()
 *
 * @brief   stop rf operation
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Rf_Stop( void );

 /***************************************************************************************************
 * @fn      Rf_IsSendFinished()
 *
 * @brief   check RF send process is finished or not
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rf_true  - rf send finished
 *          rf_false  - rf send not finished yet
 */
extern rfBool Rf_IsSendFinished( void );


 /***************************************************************************************************
 * @fn      Rf_IsReceiveFinished()
 *
 * @brief   check RF receive process is finished or not
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rf_true  - rf send finished
 *          rf_false  - rf send not finished yet
 */
extern rfBool Rf_IsReceiveFinished( void );


 /***************************************************************************************************
 * @fn      Rf_GetRssi()
 *
 * @brief   get rssi value
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rssi value
 */
extern rfUint8 Rf_GetRssi( void );


/***************************************************************************************************
 * @fn      Rf_SetPower()
 *
 * @brief   set rf power
 *
 * @author	chuanpengl
 *
 * @param   a_u8PowerVal  - rf power value
 *
 * @return  RF_RET_OK  - set success
 *          RF_RET_PARA_ERROR  - power is not suitable for this rf radio
 */
extern RF_RET_t Rf_SetPower( rfUint8 a_u8PowerVal );


/***************************************************************************************************
 * @fn      Rf_GetPower()
 *
 * @brief   get rf power value
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  power value
 */
extern rfUint8 Rf_GetPower( void );



/***************************************************************************************************
 * @fn      Rf_SetFreq()
 *
 * @brief   set rf frequence
 *
 * @author	chuanpengl
 *
 * @param   a_u32Freq  - rf frequence, unit kHz, for example, 433Mhz, a_u32Freq = 433000
 *
 * @return  RF_RET_OK  - set success
 *          RF_RET_PARA_ERROR  - frequence is not suitable for this rf radio
 */
extern RF_RET_t Rf_SetFreq( rfUint32 a_u32Freq );



/***************************************************************************************************
 * @fn      Rf_GetFreq()
 *
 * @brief   get rf frequence
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rf frequence, unit is kHz, for example, 433MHz, ret value = 433000
 */
extern rfUint32 Rf_GetFreq( void );


/***************************************************************************************************
 * @fn      Rf_SetBandwidth()
 *
 * @brief   set rf bandwidth
 *
 * @author	chuanpengl
 *
 * @param   a_u32Bandwidth  - rf bandwidth, unit is kHz, for example, 500kHz, a_u32Bandwidth = 500
 *
 * @return  RF_RET_OK  - set success
 *          RF_RET_PARA_ERROR  - bandwidth is not suitable for this rf radio
 */
extern RF_RET_t Rf_SetBandwidth( rfUint32 a_u32Bandwidth );


/***************************************************************************************************
 * @fn      Rf_GetBandwidth()
 *
 * @brief   get rf bandwidth
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rf bandwidth, unit is kHz, for example, 500kHz, ret value = 500
 */
extern rfUint32 Rf_GetBandwidth( void );


/***************************************************************************************************
 * @fn      Rf_SetAirBaud()
 *
 * @brief   set rf Air Baudrate
 *
 * @author	chuanpengl
 *
 * @param   a_u32AirBaud  - rf AirBaud, unit is Hz
 *
 * @return  RF_RET_OK  - set success
 *          RF_RET_PARA_ERROR  - baud is not suitable for this rf radio
 */
extern RF_RET_t Rf_SetAirBaud( rfUint32 a_u32AirBadu);


/***************************************************************************************************
 * @fn      Rf_GetAirBaud()
 *
 * @brief   get rf Air Baudrate
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rf air baudrate, unit is Hz
 */
extern rfUint32 Rf_GetAirBaud( void );

extern rfInt8 Rf_GetPacketSnr( void );
extern double Rf_GetPacketRssi( void );

/***************************************************************************************************
 * @fn      Rf_SetWorkState()
 *
 * @brief   set rf work mode, if rf working in standby mode, it will goto setting mode immediately,
 *          else, wait, until rf goto standby mode, rf will goto setting mode.
 *
 * @author	chuanpengl
 *
 * @param   a_tState  - rf work state
 *
 * @return  RF_RET_OK  - set success
 *          RF_RET_PARA_ERROR  - work state is not suitable for this rf radio
 */
extern RF_RET_t Rf_SetWorkState( RF_STATE_t a_tState);


/***************************************************************************************************
 * @fn      Rf_GetWorkState()
 *
 * @brief   get rf work state
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  rf work state
 */
extern RF_STATE_t Rf_GetWorkState( void );

/***************************************************************************************************
 * @fn      Rf_SetStateRequset()
 *
 * @brief   set rf state request value
 *
 * @author	chuanpengl
 *
 * @param   a_tReq  - request state
 *              RF_RX  - rf receive state
 *              RF_TX  - rf transmit state
 *              RF_SLEEP  - rf sleep state
 *              RF_CAD  - rf cad(channel activity detection) state 
 *              RF_CONFIG  - rf config state
 *          a_u16Par  - addition parameter, for RF_TX, this is the length request to send
 * @return  none
 */
extern void Rf_SetStateRequset( RF_STATE_t a_tReq, rfUint16 a_u16Par );


/***************************************************************************************************
 * @fn      Rf_UpdateMilisecond()
 *
 * @brief   update every milisecond, used as timer
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_UpdateMilisecond( void );


/***************************************************************************************************
 * @fn      Rf_ExcuteConfigParameters()
 *
 * @brief   excute rf config parameter
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Rf_ExcuteConfigParameters( void );


/***************************************************************************************************
 * @fn      Rf_GetConfigUartBaud()
 *
 * @brief   get rf config parameter uart baud
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  baudrate
 */
extern rfUint32 Rf_GetConfigUartBaud( void );

/***************************************************************************************************
 * @fn      Rf_GetConfigUartParity()
 *
 * @brief   get rf config parameter uart parity
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  parity
 */
extern rfUint8 Rf_GetConfigUartParity( void );


/***************************************************************************************************
 * @fn      Rf_GetConfigWakeupTime()
 *
 * @brief   get config wakeup time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern rfUint16 Rf_GetConfigWakeupTime( void );


/***************************************************************************************************
 * @fn      Rf_GetConfigPreamble()
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
extern rfUint16 Rf_GetConfigPreamble(rfBool a_bWakeupMode);


/***************************************************************************************************
 * @fn      Rf_SetPreambleLengthPara()
 *
 * @brief   set preamble length for parameter
 *
 * @author	chuanpengl
 *
 * @param   a_u16Length  - paramble length
 *
 * @return  none
 */
void Rf_SetPreambleLengthPara(rfUint16 a_u16length);


enum rf_state Rf_GetCurState(void);


#endif /* __RF_MANAGER_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
