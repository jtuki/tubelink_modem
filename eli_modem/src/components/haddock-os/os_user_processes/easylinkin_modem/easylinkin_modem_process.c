/***************************************************************************************************
* @file name	easylinkin_modem_process.c
* @data   		2015/04/27
* @auther   	chuanpengl
* @module   	easylinkin_modem processes
* @brief 		processes of easylinkin_modem, include hostif process,
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "easylinkin_modem_process.h"
#include "hdk_user_config.h"
#include "process_hostif.h"
#if defined MODEM_FOR_END_DEVICE
#include "end_device/mac_engine.h"
#include "process_test_end_device_app.h"
#elif defined MODEM_FOR_GATEWAY
#include "gateway/mac_engine.h"
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
 
 
/***************************************************************************************************
 * CONSTANTS
 */


/***************************************************************************************************
 * LOCAL FUNCTIONS DECLEAR
 */


/***************************************************************************************************
 * GLOBAL VARIABLES
 */


/***************************************************************************************************
 * STATIC VARIABLES
 */


/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      os_processes_init_easylinkin_modem()
 *
 * @brief   easylinkin_modem processes init
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void os_processes_init_easylinkin_modem(void) {
    proc_HostifInit();
#if defined MODEM_FOR_END_DEVICE
    device_mac_engine_init();
    proc_test_end_device_app_init();
#elif defined MODEM_FOR_GATEWAY
    gateway_mac_engine_init();
#endif
}   /* os_processes_init_easylinkin_modem */



/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
 

 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
