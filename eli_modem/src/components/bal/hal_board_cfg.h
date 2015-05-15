/***************************************************************************************************
* @file name	hal_board_cfg.h
* @data   		2014/09/01
* @auther   	author name
* @module   	module name
* @brief 		file description
***************************************************************************************************/
#ifndef __HAL_BOARD_CFG_H__
#define __HAL_BOARD_CFG_H__
/***************************************************************************************************
 * INCLUDES
 */

/***************************************************************************************************
 * DEBUG SWITCH MACROS
 */

/***************************************************************************************************
 * MACROS
 */

/* switch define for each board abstraction layer module 
 */

/* system tick module switch define
 * 0 - disable system tick
 * 1 - enable system tick
 */
#ifndef _SW_SYSTICK
#define _SW_SYSTICK        (1)
#include "systick/hal_systick.h"

#endif


/* uart module switch define
 * 0 - disable uart
 * 1 - enable uart 
 */
#ifndef _SW_UART
#define _SW_UART            (1)
#define HAL_UART_CNT        (1)
#define HAL_UART_1_EN       (1)
#define HAL_UART_2_EN       (0)
#include "uart/hal_uart.h"

#endif





/* switch define end */


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



#endif /* __HAL_BOARD_CFG_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 2. Modify Hal_SpiInit by author @ data
*  	context: modified context
*
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/