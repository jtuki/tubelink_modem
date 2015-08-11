/***************************************************************************************************
* @file name	sleep.h
* @data   		2015/04/17
* @auther   	chuanpengl
* @module   	sleep module
* @brief 		sleep module
***************************************************************************************************/
#ifndef __SLEEP_H__
#define __SLEEP_H__
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


/***************************************************************************************************
 * TYPEDEFS
 */
typedef unsigned char slpUint8;
typedef unsigned short slpUint16;
typedef enum{slpDisable = 0, slpEnable = !slpDisable}slpControlState_e;

typedef enum{
    SLP_CLK_HSI = 0,
    SLP_CLK_LSI = 1,
    SLP_CLK_HSE = 2,
    SLP_CLK_LSE = 3
}SlpClk_e;

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
 * @fn      slp_Request()
 *
 * @brief   request to sleep for some time
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void slp_Request( slpUint16 a_u16SleepTime, slpUint8 *a_pu8SlpAssert );

#ifdef __cplusplus
}
#endif

#endif /* __SLEEP_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
