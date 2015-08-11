/***************************************************************************************************
* @file name	sx1276_config.c
* @data   		2014/12/08
* @auther   	chuanpengl
* @module   	sx1276 configure
* @brief 		do sx1276 config process
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include <string.h>
#include "stm8l15x.h"
#include "sx1276_config.h"
#include "utilities\crc\crc.h"
#include "sx1276_lora.h"

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define u16RF_CONFIG_BUFFER_SIZE   (64)
/* when in config mode, config data must be receive in 100 milisecond */
#define u16RF_CONFIG_ALIVE_TIMEOUT      (100)

#define STATE_NORMAL_PREAMBLE_LENGTH    (8)
#define WAKEUP_TIME_PREAMBLE()       \
    (gs_tLoRaCfg.u8WakeupTime < PREAMBLE_LENGTH_SELECT ? gs_au16PreambleLength[gs_tLoRaCfg.u8WakeupTime] : gs_au16PreambleLength[0])

#define PREAMBLE_LENGTH_SELECT          (0x0C)
#define WAKEUP_TIME_SLECT               PREAMBLE_LENGTH_SELECT
#define GET_TIME_LAPSE(time_now, time_start)      ((time_now >= time_start)? time_now - time_start : 0xFFFFFFFF - time_start + time_now)

/* macro for gs_tLoRaCfg */
#define LORA_CONFIG_DATA_LENGTH     (11)      
#define LORA_CONFIG_DATA_START      (3)

/* macro for command length */        
#define LORA_WRITE_CMD_LENGTH       (strlen((rf_char const*)gsc_au8WriteCmd))
#define LORA_READ_CMD_LENGTH        (strlen((rf_char const*)gsc_au8ReadCmd)) 
        
        

/***************************************************************************************************
 * CONSTANTS
 */
/* used for normal mode, if wakeup mode, calculate the value */
const rfUint16 gsc_au16PreambleLength[]=
{
    8, 8, 8, 12, 12, 12
};

const rfUint16 gsc_au16SymbolTimeUs[] = 
{
    8192, 4096, 2048, 1024, 512, 256
};

const rfUint8 gsc_au8TxPower[] = 
{
    3,6,9,11,13,15,18,20
};


/* wakeup time with unit 50ms */
const rfUint8 gs_au8WakeupTime[WAKEUP_TIME_SLECT] = 
{
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12
};

const rfUint16 gs_au8SF[6] = {10,9,8,7,7,7};
const rfUint8 gs_au8SigBw[6] = {7,7,7,7,8,9};



const rfUint8 gsc_au8ReadCmd[] = {0xFF,0x56,0xAE,0x35,0xA9,0x55,0xF0, 0x00};  /* configure read command */
const rfUint8 gsc_au8WriteCmd[] = {0xFF,0x56,0xAE,0x35,0xA9,0x55, 0x90, 0x00};
const rfUint16 gsc_au16WakeupTime[] = {
    50,    /* 50ms */
    100,    /* 100ms */
    200,   /* 200ms */
    400,   /* 400ms */
    600,   /* 600ms */
    1000,   /* 1000ms */
    1500,  /* 1500ms */
    2000,  /* 2000ms */
    2500,  /* 2500ms */
    3000,  /* 3000ms */
    4000,  /* 4000ms */
    5000   /* 5000ms */
};
const rfUint32 gsc_au16Baud[] = 
{
    1200,2400,4800,9600,19200,38400,57600,115200
};

/* tx packet timeout value */
static const rfUint32 gs_u32TX_PACKET_TIMEOUT[] = 
{
    3000, 2000, 1000, 1000, 500, 500
};

const rfUint32 eeprom_start_add = (uint32_t)FLASH_DATA_EEPROM_START_PHYSICAL_ADDRESS + (0 * FLASH_BLOCK_SIZE);  /* block 0 of eeprom */


/***************************************************************************************************
 * TYPEDEFS
 */
typedef struct
{
    rfUint8 u8Ack;          /* ACK byte */
    rfUint8 u8Type;         /* module type */
    rfUint8 u8Version;      /* module version */
    rfUint8 au8Freq[3];     /* frequency */
    rfUint8 u8AirRate;      /* the rate in the are */
    rfUint8 u8TxPower;      /* transmit power */
    rfUint8 u8UartBaud;     /* uart baudrate */
    rfUint8 u8UartParity;   /* uart parity:none,even,odd */
    rfUint8 u8WakeupTime;   /* wake up time */  
}LORA_CONFIG_TYPE_t;


typedef enum
{
   RF_CFG_READ = 0,
   RF_CFG_WRITE = 1,
   RF_CFG_INVALID_PARA = 2
}RF_CONFIG_RET_t;



/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */

static rfUint8 gs_u8ConfigBuffer[u16RF_CONFIG_BUFFER_SIZE] = {0};  /* config buffer */
static rfUint8 gs_u8ConfigBufSize = 0;     /* config buffer size */

static rfUint16 gs_u16RfConfigAliveTime = 0;   /* rf config alive time */

static LORA_CONFIG_TYPE_t gs_tLoRaCfg = {
    0x24,   /* ACK byte */
    0x0E,   /* module type */
    0x10,   /* module version */
    0x06,   /* frequency high byte, default = 434MHz */
    0x9F,   /* frequency middle byte */
    0x50,   /* frequency low byte */
    0x04,   /* the rate in the air, defalut = 9.11k */
    0x03,   /* transmit power, default = lv3 */
    0x05,   /* uart baudrate, default = 38400 */
    0x00,   /* uart parity:none,even,odd, default = none */
    0x00    /* wake up time, default = 50ms */  
};




/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */
/***************************************************************************************************
 * @fn      Sx1276_Cfg_ClearAliveTime__()
 *
 * @brief   clear config alive time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
inline static void Sx1276_Cfg_ClearAliveTime__( void );


/***************************************************************************************************
 * @fn      Sx1276_Cfg_ConfigProcess__()
 *
 * @brief   do sx1276 configure process
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static RF_CONFIG_RET_t Sx1276_Cfg_ConfigProcess__(void);

/***************************************************************************************************
 * @fn      Sx1276_Cfg_ReadPara__()
 *
 * @brief   read sx1276 parameter from nv
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
static void Sx1276_Cfg_ReadPara__(void);

 
/***************************************************************************************************
 *  EXTERNAL FUNCTIONS IMPLEMENTATION
 */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_Init()
 *
 * @brief   do sx1276 init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Sx1276_Cfg_Init( void )
{
    Sx1276_Cfg_ClearAliveTime__();
} /* Sx1276_Cfg_Init() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_Process()
 *
 * @brief   do sx1276 process
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
RF_P_RET_t Sx1276_Cfg_Process( void )
{
    RF_P_RET_t tRet = RF_P_RUNNING;
    RF_CONFIG_RET_t tConfigResult = RF_CFG_READ;
    
    /* if config timeout, return RF_P_CFG_TIMEOUT */
    if(gs_u16RfConfigAliveTime > u16RF_CONFIG_ALIVE_TIMEOUT)
    {
        tRet = RF_P_CFG_TIMEOUT;
    }
    /* do config process */
    else
    {
        tConfigResult = Sx1276_Cfg_ConfigProcess__();
        
        if(RF_CFG_INVALID_PARA == tConfigResult)
        {
            tRet = RF_P_CFG_INVALID_PARA;
        }
        else
        {
            tRet = RF_P_OK;
        }
    }
    
    
    return tRet;
} /* Sx1276_Cfg_Process() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetBufferAddr()
 *
 * @brief   get config buffer Address
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  buffer address
 */
void* Sx1276_Cfg_GetBufferAddr( void )
{
    return gs_u8ConfigBuffer;

} /* Sx1276_Cfg_GetBufferAddr() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_SetBufferSize()
 *
 * @brief   set config buffer size
 *
 * @author	chuanpengl
 *
 * @param   a_u8Size  - config buffer size
 *
 * @return  none
 */
void Sx1276_Cfg_SetBufferSize( rfUint8 a_u8Size )
{
    gs_u8ConfigBufSize = a_u8Size;

} /* Sx1276_Cfg_SetBufferSize() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetBufferSize()
 *
 * @brief   set config buffer size
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  config buffer size
 */
rfUint8 Sx1276_Cfg_GetBufferSize( void )
{
    return gs_u8ConfigBufSize;

} /* Sx1276_Cfg_GetBufferSize() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_UpdateAliveTime()
 *
 * @brief   update config alive time
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Sx1276_Cfg_UpdateAliveTime( void )
{
    gs_u16RfConfigAliveTime ++;
}/* Sx1276_Cfg_UpdateAliveTime() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_ExcuteParameters()
 *
 * @brief   excute parameter
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Sx1276_Cfg_ExcuteParameters( void )
{
    Sx1276_Cfg_ReadPara__();
    
    SX1276LoRaSetRFBaseFrequency(((((uint32_t)gs_tLoRaCfg.au8Freq[0]) << 16 )| (((uint32_t)gs_tLoRaCfg.au8Freq[1]) << 8 )| (uint32_t)gs_tLoRaCfg.au8Freq[2]) * 1000);
    SX1276LoRaSetSignalBandwidth(gs_au8SigBw[gs_tLoRaCfg.u8AirRate]);
    SX1276LoRaSetSpreadingFactor(gs_au8SF[gs_tLoRaCfg.u8AirRate]);
    SX1276LoRaSetTxPacketTimeout(gs_u32TX_PACKET_TIMEOUT[gs_tLoRaCfg.u8AirRate]);
     
    switch(gs_tLoRaCfg.u8AirRate)
    {
        case 0:
        SX1276LoRaSetErrorCoding(2);
        break;
        case 1:
        SX1276LoRaSetErrorCoding(2);
        break;
        case 2:
        SX1276LoRaSetErrorCoding(2);
        break;
        case 3:
        SX1276LoRaSetErrorCoding(2);
        break;
        case 4:
        SX1276LoRaSetErrorCoding(3);
        break;
        case 5:
        SX1276LoRaSetErrorCoding(4);
        break;
        default:
        
        break;
    }
    
    SX1276LoRaSetRFPower(gsc_au8TxPower[gs_tLoRaCfg.u8TxPower]);
}   /* Sx1276_Cfg_ExcuteParameters() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetUartBaud()
 *
 * @brief   get config uart baud
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
rfUint32 Sx1276_Cfg_GetUartBaud( void )
{
    return gsc_au16Baud[gs_tLoRaCfg.u8UartBaud];
}   /* Sx1276_Cfg_GetUartBaud() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetUartParity()
 *
 * @brief   get config uart parity
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
rfUint8 Sx1276_Cfg_GetUartParity( void )
{
    return gs_tLoRaCfg.u8UartParity;
}   /* Sx1276_Cfg_GetUartParity() */

/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetWakeupTime()
 *
 * @brief   get config wakeup time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
rfUint16 Sx1276_Cfg_GetWakeupTime( void )
{
    return gsc_au16WakeupTime[gs_tLoRaCfg.u8WakeupTime];
}   /* Sx1276_Cfg_GetWakeupTime() */

/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetPreamble()
 *
 * @brief   get config get preamble
 *
 * @author  chuanpengl
 *
 * @param   a_bWakeupMode  - rf_true = wakeup mode
 *                         - rf_false = normal mode
 *
 * @return  none
 */
rfUint16 Sx1276_Cfg_GetPreamble(rfBool a_bWakeupMode)
{
    if(rf_true == a_bWakeupMode)
    {
        return (rfUint16)((rfUint32)gsc_au16WakeupTime[gs_tLoRaCfg.u8WakeupTime]*1000 / gsc_au16SymbolTimeUs[gs_tLoRaCfg.u8AirRate]);
    }
    else
    {
        return gsc_au16PreambleLength[gs_tLoRaCfg.u8AirRate];
    }
}   /* Sx1276_Cfg_GetPreamble() */

/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetPreambleTime()
 *
 * @brief   get config get preamble
 *
 * @author  chuanpengl
 *
 * @param   a_bWakeupMode  - rf_true = wakeup mode
 *                         - rf_false = normal mode
 *
 * @return  none
 */
rfUint16 Sx1276_Cfg_GetPreambleTime(rfBool a_bWakeupMode)
{
    if(rf_true == a_bWakeupMode)
    {
        return gsc_au16WakeupTime[gs_tLoRaCfg.u8WakeupTime] + 1;
    }
    else
    {
        return gsc_au16PreambleLength[gs_tLoRaCfg.u8AirRate]*gsc_au16SymbolTimeUs[gs_tLoRaCfg.u8AirRate] / 1000 + 1;
    }
}   /* Sx1276_Cfg_GetPreambleTime() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetSymbolTime()
 *
 * @brief   get config get preamble
 *
 * @author  chuanpengl
 *
 * @param   a_bWakeupMode  - rf_true = wakeup mode
 *                         - rf_false = normal mode
 *
 * @return  none
 */
rfUint16 Sx1276_Cfg_GetSymbolTime(void)
{
    return gsc_au16SymbolTimeUs[gs_tLoRaCfg.u8AirRate] / 1000 + 1;

}   /* Sx1276_Cfg_GetSymbolTime() */

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */ 
/***************************************************************************************************
 * @fn      Sx1276_Cfg_ClearAliveTime__()
 *
 * @brief   clear config alive time
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Sx1276_Cfg_ClearAliveTime__( void )
{
    __disable_interrupt();
    gs_u16RfConfigAliveTime = 0;
    __enable_interrupt();
}/* Sx1276_Cfg_ClearAliveTime__() */


/***************************************************************************************************
 * @fn      Sx1276_Cfg_ConfigProcess__()
 *
 * @brief   do sx1276 configure process
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
RF_CONFIG_RET_t Sx1276_Cfg_ConfigProcess__(void)
{
    RF_CONFIG_RET_t tRet;
    rf_char *pAddress = rf_null;
    
    rfUint8 i = 0;
    
    if(0 < gs_u8ConfigBufSize)
    {
        
        /*  read command */
        pAddress = strstr((rf_char const *)gs_u8ConfigBuffer, (rf_char const *)gsc_au8ReadCmd);
        
        if(rf_null != pAddress)     /* receive read command */
        {
            memcpy(gs_u8ConfigBuffer, (rfUint8*)&gs_tLoRaCfg, LORA_CONFIG_DATA_LENGTH);
            gs_u8ConfigBufSize = LORA_CONFIG_DATA_LENGTH;
            tRet = RF_CFG_READ;
        }
        /* write command */
        else
        {
            pAddress = strstr((rf_char const *)gs_u8ConfigBuffer, (rf_char const *)gsc_au8WriteCmd);
            
            if( rf_null != pAddress )   /* receive write command */
            {
            
                memcpy(((rf_char*)&gs_tLoRaCfg) + LORA_CONFIG_DATA_START, pAddress + LORA_WRITE_CMD_LENGTH, LORA_CONFIG_DATA_LENGTH - LORA_CONFIG_DATA_START);
                memcpy(gs_u8ConfigBuffer, (rfUint8*)&gs_tLoRaCfg, LORA_CONFIG_DATA_LENGTH);
                gs_u8ConfigBufSize = LORA_CONFIG_DATA_LENGTH;

                /* flash write operation */
                FLASH_SetProgrammingTime(FLASH_ProgramTime_Standard);
                FLASH_Unlock(FLASH_MemType_Data);

                while (FLASH_GetFlagStatus(FLASH_FLAG_DUL) == RESET)
                {}

                for (i = 0; i < LORA_CONFIG_DATA_LENGTH; i++)
                {
                    FLASH_ProgramByte(eeprom_start_add+i, ((rfUint8*)&gs_tLoRaCfg)[i]);
                    FLASH_WaitForLastOperation(FLASH_MemType_Data);
                }
                
                /* write crc data */
                FLASH_ProgramByte(eeprom_start_add+i, crc8bit((rfUint8*)&gs_tLoRaCfg, LORA_CONFIG_DATA_LENGTH));
                FLASH_WaitForLastOperation(FLASH_MemType_Data);

                while (FLASH_GetFlagStatus(FLASH_FLAG_HVOFF) == RESET)
                {}

                FLASH_Lock(FLASH_MemType_Data);
                
                tRet = RF_CFG_WRITE;
            }
            else
            {
                tRet = RF_CFG_INVALID_PARA;
            }
        }
    }
    
    return tRet;
}


/***************************************************************************************************
 * @fn      Sx1276_Cfg_ReadPara__()
 *
 * @brief   read sx1276 parameter from nv
 *
 * @author  chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void Sx1276_Cfg_ReadPara__(void)
{
    rfUint8 i = 0;
    rfUint8 u8ReadData[LORA_CONFIG_DATA_LENGTH + 1] = {0};
    for (i = 0; i < LORA_CONFIG_DATA_LENGTH + 1; i++)
    {
        u8ReadData[i] = FLASH_ReadByte(eeprom_start_add+i);
    }
    
    
    /* if crc value is right, update configure data */
    if((0 == crc8bit(u8ReadData, LORA_CONFIG_DATA_LENGTH+1)) && (0x24 == u8ReadData[0]) && (0x0E == u8ReadData[1]))
    {
        memcpy((rfUint8*)&gs_tLoRaCfg, u8ReadData, LORA_CONFIG_DATA_LENGTH);
    }
    /* if crc value is error, ignore */
    else
    {
    }
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