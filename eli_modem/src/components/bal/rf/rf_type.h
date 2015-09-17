/***************************************************************************************************
* @file name	file name.h
* @data   		2014/09/01
* @auther   	author name
* @module   	module name
* @brief 		file description
***************************************************************************************************/
#ifndef __RF_TYPE_H__
#define __RF_TYPE_H__
/***************************************************************************************************
 * INCLUDES
 */


/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */
#define rf_null      ((void *)0 )

/***************************************************************************************************
 * CONSTANTS
 */



/***************************************************************************************************
 * TYPEDEFS
 */
typedef unsigned char       rf_uint8;
typedef signed char         rf_int8;
typedef char                rf_char;
typedef unsigned short      rf_uint16;
typedef signed short        rf_int16;
typedef unsigned long       rf_uint32;
typedef signed long         rf_int32;


typedef enum{rf_false = 0, rf_true = 1} rf_bool;

/***************************************************************************************************
 * GLOBAL VARIABLES DECLEAR
 */


/***************************************************************************************************
 * EXTERNAL FUNCTIONS DECLEAR
 */

 
/***************************************************************************************************
 * @fn      Hal_SpiInit()
 *
 * @brief   Initial SPI Driver
 *
 * @author	author name
 *
 * @param   none
 *
 * @return  none
 */
extern void Hal_SpiInit( void );


#endif /* __RF_TYPE_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
