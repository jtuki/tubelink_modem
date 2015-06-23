/***************************************************************************************************
* @file name	rf_manager.c
* @data   		2014/11/14
* @auther   	chuanpengl
* @module   	rf manager
* @brief 		rf manager, provide apis
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */

#include "kernel/hdk_memory.h"

#include "stm8l15x.h"
#include "rf_manager.h"
#include "rf_config.h"

//#include "app_dtu.h"
/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define u16RF_TX_BUFFER_SIZE            (128)


/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */
typedef enum
{
    BUFFER_EMPTY = 0,
    BUFFER_LOADING = 1,
    BUFFER_UNLOADING = 2,
    BUFFER_READY = 3
}BUFFER_STATE_t;


typedef struct
{
    rf_uint8 u8Select;
    BUFFER_STATE_t tBufferSt[2];
    rf_uint8 au8BufferIdx[2];
    rf_uint8 au8BufferSize[2];
    rf_char acBuffer[2][u16RF_TX_BUFFER_SIZE];
}PING_PONG_BUFFER_t;

/* rf state request struct */
typedef struct
{
    rf_uint8 bitConfigReq:1;
    rf_uint8 bitSleepReq:1;
    rf_uint8 bitTxReq:1;
    rf_uint8 bitRxReq:1;
    rf_uint8 bitCadReq:1;
}RF_STATE_REQ_t;


/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */
/* rf state */
static RF_STATE_t gs_tRfState = RF_STANDBY;

/* rf state request variable */
static RF_STATE_REQ_t gs_tRfStateReq;


static Rf_Event gs_hRfEvt = (void*)0;
/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */

/***************************************************************************************************
 * @fn      Rf_GetStateRequset__()
 *
 * @brief   get rf state request value
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  RF_RX  - rf receive state
 *          RF_TX  - rf transmit state
 *          RF_SLEEP  - rf sleep state
 *          RF_CAD  - rf cad(channel activity detection) state 
 *          RF_CONFIG  - rf config state
 */
static RF_STATE_t Rf_GetStateRequset__( void );



/***************************************************************************************************
 *  EXTERNAL FUNCTIONS IMPLEMENTATION
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
void Rf_Init( Rf_Event a_hEvt )
{
    /* call init functions - init function will init rf module to standby state */
    Rf_P_Init();
    
    gs_hRfEvt = a_hEvt;
    
    /* set rf state standby */
    gs_tRfState = RF_STANDBY;
}/* RF_Init() */



/**
 * Get current rf-manager's state.
 */
enum rf_state Rf_GetCurState(void)
{
    return gs_tRfState;
}

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
void Rf_WakeUpInit( void )
{
    Rf_P_WakeUpInit();
    Rf_SetPreambleLengthPara(Rf_GetConfigPreamble(rf_true));
    /* set rf state standby */
    gs_tRfState = RF_STANDBY;
}   /* Rf_WakeUpInit() */





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
RF_RTE_RET_t Rf_Routine( void *a_pvdArg )
{
    RF_RTE_RET_t tRet = RF_RTE_RUN;
    RF_P_RET_t tResult = RF_P_RUNNING;
    
    switch( gs_tRfState )
    {
        case RF_STANDBY:
            //Debug_WorkState();
            gs_tRfState = Rf_GetStateRequset__();
            /* if sleep request, goto sleep state */
            if(RF_SLEEP == gs_tRfState)
            {
                Rf_P_SleepInit();   /* Sleep init */
            }
            /* if has data need to be send, goto send state */
            else if(RF_TX == gs_tRfState)
            {              
                Rf_P_TxInit();
            }
            /* if cad request, goto cad state */
            else if(RF_CAD == gs_tRfState)
            {
                Rf_P_CADInit();
                gs_tRfState = RF_CAD;
            }
            /* else, goto rx state */
            else if(RF_RX == gs_tRfState)
            {
                Rf_P_RxInit();
                gs_tRfState = RF_RX;
            }
            else
            {
                gs_hRfEvt(RF_EVT_IDLE);
                tRet = RF_RTE_STANDBY;
            }
        break;
        
        case RF_RX:
            /* do rx process */
            tResult = Rf_P_RxProcess();
            
            /* if receive ok */
            if(RF_P_OK == tResult)
            {
                /* update data to buffer */
//                Dtu_GetRfBuffer(Rf_P_GetRxBufferAddress(), Rf_P_GetRxPacketSize());
                
                gs_tRfState = RF_STANDBY;
                
                if(gs_hRfEvt)
                {
                    gs_hRfEvt(RF_EVT_RX_OK);
                }
                
                
                tRet = RF_RTE_RX_END;
            }
            /* if rx timeout, goto rf standby state */
            else if(RF_P_RX_TMO == tResult)
            {
                gs_tRfState = RF_STANDBY;
                
                if(gs_hRfEvt)
                {
                    gs_hRfEvt(RF_EVT_RX_TMO);
                }
                
                
                tRet = RF_RTE_RX_END;
            }
            /* if rx crc error, goto rf standby state */
            else if(RF_P_RX_CRC_ERR == tResult)
            {
                gs_tRfState = RF_STANDBY;
                
                if(gs_hRfEvt)
                {
                    gs_hRfEvt(RF_EVT_RX_CRCERR);
                }
                
                tRet = RF_RTE_RX_END;
            }
            
        break;
        
        case RF_TX:
            /* start tx process */
            tResult = Rf_P_TxProcess();
            
            /* transmit success */
            if(RF_P_OK == tResult)
            { 
                gs_tRfState = RF_STANDBY;
                
                if(gs_hRfEvt)
                {
                    gs_hRfEvt(RF_EVT_TX_OK);
                }
               
            }
            /* if tx timeout, goto rf standby state */
            else if(RF_P_TX_TMO == tResult)
            {
                gs_tRfState = RF_STANDBY;
                
                if(gs_hRfEvt)
                {
                    gs_hRfEvt(RF_EVT_TX_TMO);
                }
                
            }
        break;
        
        case RF_CAD:
            /* cad process */
            tResult = Rf_P_CADProcess();
            
            /* cad detected */
            if(RF_P_OK == tResult)
            {
                gs_tRfState = RF_STANDBY;
                tRet = RF_RTE_CAD_DETECTED;
            }
            else if(RF_P_CAD_EMPTY == tResult)
            {
                gs_tRfState = RF_STANDBY;
                tRet = RF_RTE_CAD_EMPTY;
            }
            
            
        break;
        
        case RF_SLEEP:
            tResult = Rf_P_SleepProcess();
            
            /* rf sleep mode ready */
            if(RF_P_OK == tResult)
            {
                tRet = RF_RTE_SLEEP;
                gs_tRfState = RF_STANDBY;   /* next state is standby */
            }
        /* when wakeup, goto standby mode */
        break;
        
        case RF_CONFIG:     
            /* do rf config process */
            //tResult = Rf_Cfg_Process();
            
            gs_tRfState = RF_STANDBY;   /* next state is standby */
            
            if(RF_P_OK == tResult)
            {
//                Dtu_GetRfBuffer(Rf_Cfg_GetBufferAddr(), Rf_Cfg_GetBufferSize());
                tRet = RF_RTE_CFG_SUCCESS;
            }
            
            else if(RF_P_CFG_TIMEOUT == tResult)
            {
                tRet = RF_RTE_CFG_EXIT;
            }
            else
            {
                tRet = RF_RTE_CFG_FAILED;
            }
            
            

        break;
        
        default:
        /* if unknown state, do init, and goto standby state */
        gs_tRfState = RF_STANDBY;
        break;
    }
    
    return tRet;
} /* Rf_Routine() */


 
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
RF_RET_t Rf_Send( rf_char *a_pcData, rf_uint16 a_u16Len )
{
    if(a_pcData)
    {
        if(a_u16Len > 128)
        {
            a_u16Len = 128;
        }
        
        haddock_memcpy(Rf_P_GetTxBufferAddress(), a_pcData, a_u16Len);
        Rf_P_SetTxPacketSize(a_u16Len);
        
        Rf_SetStateRequset(RF_TX, 0);
    }

    return RF_RET_OK;
}/* Rf_Send() */


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
rf_uint16 Rf_Get( rf_char *a_pcData, rf_uint16 a_u16Len )
{
    rf_uint16 u16Ret = 0;
    if(a_pcData)
    {
        u16Ret = Rf_P_GetRxPacketSize();
        haddock_memcpy(a_pcData, Rf_P_GetRxBufferAddress(), u16Ret);
        Rf_SetStateRequset(RF_STANDBY, 0);
    }
    return u16Ret;
}/* Rf_Get() */

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
void Rf_ReceiveStart( void )
{
    Rf_SetStateRequset(RF_RX, 0);
}   /* Rf_ReceiveStart() */

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
void Rf_Stop( void )
{
    Rf_P_Stop();
    gs_tRfState = RF_STANDBY;
}   /* Rf_Stop() */



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
rf_bool Rf_IsSendFinished( void )
{
    return rf_true;
}/* Rf_IsSendFinished() */


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
rf_bool Rf_IsReceiveFinished( void )
{
    return rf_true;
}/* Rf_IsReceiveFinished() */


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
rf_uint8 Rf_GetRssi( void )
{
    return 0;
}/* Rf_GetRssi() */


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
RF_RET_t Rf_SetPower( rf_uint8 a_u8PowerVal )
{
    Rf_P_SetRFPower( (rf_int8)(a_u8PowerVal & 0x7F) );
    return RF_RET_OK;
}/* Rf_SetPower() */


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
rf_uint8 Rf_GetPower( void )
{
    return 0;
}/* Rf_GetPower() */



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
RF_RET_t Rf_SetFreq( rf_uint32 a_u32Freq )
{
    return RF_RET_OK;
}/* Rf_SetFreq() */



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
rf_uint32 Rf_GetFreq( void )
{
    return 0;
}/* Rf_GetFreq() */


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
RF_RET_t Rf_SetBandwidth( rf_uint32 a_u32Bandwidth )
{
    return RF_RET_OK;
}/* Rf_SetBandwidth() */


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
rf_uint32 Rf_GetBandwidth( void )
{
    return 0;
}/* Rf_GetBandwidth() */


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
RF_RET_t Rf_SetAirBaud( rf_uint32 a_u32AirBadu)
{
    return RF_RET_OK;
}/* Rf_SetAirBaud() */


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
rf_uint32 Rf_GetAirBaud( void )
{
    return 0;
}/* Rf_GetAirBaud() */


rf_int8 Rf_GetPacketSnr( void ){
    return SX1276LoRaGetPacketSnr();
}

double Rf_GetPacketRssi( void ){
    return SX1276LoRaGetPacketRssi();
}

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
RF_RET_t Rf_SetWorkState( RF_STATE_t a_tState)
{
    return RF_RET_OK;
}/* Rf_SetWorkState() */


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
RF_STATE_t Rf_GetWorkState( void )
{
    return RF_STANDBY;
}/* Rf_GetWorkState() */       
 

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
void Rf_SetStateRequset( RF_STATE_t a_tReq, rf_uint16 a_u16Par )
{
    *(rf_uint8*)&gs_tRfStateReq = 0;
    switch( a_tReq )
    {
        case RF_RX:
            gs_tRfStateReq.bitRxReq = 1;
        break;
        case RF_TX:
            gs_tRfStateReq.bitTxReq = 1;
        break;
        case RF_SLEEP:
            gs_tRfStateReq.bitSleepReq = 1;
        break;
        case RF_CAD:
            gs_tRfStateReq.bitCadReq = 1;
        break;
        case RF_CONFIG:
            gs_tRfStateReq.bitConfigReq = 1;
        break;
        default:
        break;
    }
}   /* Rf_SetStateRequset() */


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
void Rf_UpdateMilisecond( void )
{
    Rf_P_UpdateTxPacketTime();
    Rf_Cfg_UpdateAliveTime();
}/* Rf_UpdateMilisecond() */   


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
void Rf_ExcuteConfigParameters( void )
{
    Rf_Cfg_ExcuteParameters();
}   /* Rf_ExcuteConfigParameters() */


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
rf_uint32 Rf_GetConfigUartBaud( void )
{
    return Rf_Cfg_GetUartBaud();
}   /* Rf_GetConfigUartBaud() */


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
rf_uint8 Rf_GetConfigUartParity( void )
{
    return Rf_Cfg_GetUartParity();
}   /* Rf_GetConfigUartParity() */


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
rf_uint16 Rf_GetConfigWakeupTime( void )
{
    return Rf_Cfg_GetWakeupTime();
}   /* Rf_GetConfigWakeupTime() */


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
rf_uint16 Rf_GetConfigPreamble(rf_bool a_bWakeupMode)
{
    return Rf_Cfg_GetPreamble(a_bWakeupMode);
}   /* Rf_GetConfigPreamble() */


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
void Rf_SetPreambleLengthPara(rf_uint16 a_u16length)
{
    Rf_P_SetPreambleLengthPara(a_u16length);
}   /* Rf_SetPreambleLengthPara() */

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      Rf_GetStateRequset__()
 *
 * @brief   get rf state request value
 *
 * @author	chuanpengl
 *
 * @param   none 
 *
 * @return  RF_RX  - rf receive state
 *          RF_TX  - rf transmit state
 *          RF_SLEEP  - rf sleep state
 *          RF_CAD  - rf cad(channel activity detection) state 
 *          RF_CONFIG  - rf config state
 */
RF_STATE_t Rf_GetStateRequset__( void )
{
    RF_STATE_t tRet = RF_STANDBY;
    
    if(1 == gs_tRfStateReq.bitConfigReq)
    {
        tRet = RF_CONFIG;
        gs_tRfStateReq.bitConfigReq = 0;
    }
    else if(1 == gs_tRfStateReq.bitTxReq)
    {
        tRet = RF_TX;
        gs_tRfStateReq.bitTxReq = 0;
    }
    else if(1 == gs_tRfStateReq.bitRxReq)
    {
        tRet = RF_RX;
        gs_tRfStateReq.bitRxReq = 0;
    }
    else if(1 == gs_tRfStateReq.bitSleepReq)
    {
        tRet = RF_SLEEP;
        gs_tRfStateReq.bitSleepReq = 0;
    }
    else if(gs_tRfStateReq.bitCadReq)
    {
        tRet = RF_CAD;
        gs_tRfStateReq.bitCadReq = 0;
    }
    else
    {
        tRet = RF_STANDBY;
    }
    return tRet;
}





/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
