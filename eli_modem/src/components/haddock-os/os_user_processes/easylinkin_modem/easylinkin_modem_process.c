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
#if defined (MODEM_FOR_END_DEVICE) && MODEM_FOR_END_DEVICE == OS_TRUE
#include "radio_controller/radio_controller.h"
#include "end_device/mac_engine.h"
#include "process_test_end_device_app.h"
#elif defined (MODEM_FOR_GATEWAY) && MODEM_FOR_GATEWAY == OS_TRUE
#include "gateway/mac_engine.h"
#include "gateway/mac_engine_driver.h"
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
#if defined (MODEM_FOR_END_DEVICE) && MODEM_FOR_END_DEVICE == OS_TRUE
    /** same priority of radio controller and mac engine, init radio controller first */
    proc_HostifInit(1);     // higher the value, lower the priority
    radio_controller_init(0);
    device_mac_engine_init(0);
    
#ifdef LPWAN_DEBUG_ONLY_TRACK_BEACON
#else
    proc_test_end_device_app_init(2);
#endif // LPWAN_DEBUG_ONLY_TRACK_BEACON
    
#elif defined (MODEM_FOR_GATEWAY) && MODEM_FOR_GATEWAY == OS_TRUE
    proc_HostifInit(1);
    gateway_mac_engine_init(0);
    gateway_mac_engine_driver_init(1);
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
