/***************************************************************************************************
* @file name	sx1276_config.h
* @data   		2014/12/08
* @auther   	chuanpengl
* @module   	sx1276 configure
* @brief 		realize sx1276 config process
***************************************************************************************************/
#ifndef __SX1276_CONFIG_H__
#define __SX1276_CONFIG_H__
/***************************************************************************************************
 * INCLUDES
 */
#include <stdio.h>
#include "rf\rf_type.h"
#include "../rf_config.h"

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


/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * EXTERNAL FUNCTIONS DECLEAR
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
extern void Sx1276_Cfg_Init( void );

 
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
extern RF_P_RET_t Sx1276_Cfg_Process( void );


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
extern void* Sx1276_Cfg_GetBufferAddr( void );


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
extern void Sx1276_Cfg_SetBufferSize( rf_uint8 a_u8Size );


/***************************************************************************************************
 * @fn      Sx1276_Cfg_UpdateAliveTime()
 *
 * @brief   update config alive time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
extern void Sx1276_Cfg_UpdateAliveTime( void );


/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetUartBaud()
 *
 * @brief   get config uart baud
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  uart baud
 */
extern rf_uint32 Sx1276_Cfg_GetUartBaud( void );

/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetUartParity()
 *
 * @brief   get config uart parity
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  uart parity
 */
extern rf_uint8 Sx1276_Cfg_GetUartParity( void );


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
extern rf_uint16 Sx1276_Cfg_GetWakeupTime( void );


/***************************************************************************************************
 * @fn      Sx1276_Cfg_GetPreamble()
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
extern rf_uint16 Sx1276_Cfg_GetPreamble(rf_bool a_bWakeupMode);



#endif /* __SX1276_CONFIG_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/