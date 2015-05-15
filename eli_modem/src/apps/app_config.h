/***************************************************************************************************
* @file name	app_config.h
* @data   		2015/04/13
* @auther   	chuanpengl
* @module   	configure module
* @brief 		configure
***************************************************************************************************/
#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__
/***************************************************************************************************
 * INCLUDES
 */


#ifdef __cplusplus
extern "C"{
#endif

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define MANUFACTURER    "NET4IOT"
#define DEVICE_TYPE     "NET4IOT TERMINAL T1.0"
#define HW_VERSION      "HW:0.1.0"
#define SW_VERSION      "SW:0.1.0"

#define CFG_MAC_LENGTH          (13)    /* 6byte mac address = 12 byte in character + end character */
#define CFG_SN_LENGTH           (17)    /* 8byte sn address = 12 byte in character + end character */

/***************************************************************************************************
 * TYPEDEFS
 */
typedef char cfgChar;
typedef unsigned char cfgUint8;

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
 * @fn      cfg_Init()
 *
 * @brief   configure initial, read configure info from NVM when start or restart
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cfg_Init( void );

/***************************************************************************************************
 * @fn      cfg_GetMac()
 *
 * @brief   get current mac address
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cfg_GetMac( cfgChar *a_pcData, cfgUint8 a_u8Length );


/***************************************************************************************************
 * @fn      cfg_GetSerialNumber()
 *
 * @brief   get current serial number
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void cfg_GetSerialNumber( cfgChar *a_pcData, cfgUint8 a_u8Length );


#ifdef __cplusplus
}
#endif

#endif /* __APP_CONFIG_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/