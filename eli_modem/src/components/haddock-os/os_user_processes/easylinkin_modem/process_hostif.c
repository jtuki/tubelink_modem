/***************************************************************************************************
* @file name	process_hostif.c
* @data   		2015/04/27
* @auther   	chuanpengl
* @module   	host if process
* @brief 		privode host interface
***************************************************************************************************/

/***************************************************************************************************
 * INCLUDES
 */
#include "kernel/process.h"
#include "kernel/timer.h"
#include "kernel/ipc.h"
#include "kernel/sys_signal_defs.h"
#include "lib/assert.h"
#include "easylinkin_modem_process.h"

#include "app_hostif.h"


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
 * @fn      proc_HostifEntry__()
 *
 * @brief   host interface process
 *
 * @author	chuanpengl
 *
 * @param   a_nPid  - process id
 *          a_bvSig  - process signal bit vector
 *
 * @return  remain bit vector
 */
static signal_bv_t proc_HostifEntry__(os_pid_t a_nPid, signal_bv_t a_bvSig);

/***************************************************************************************************
 * GLOBAL VARIABLES
 */
os_pid_t g_procHostifPid;

/***************************************************************************************************
 * STATIC VARIABLES
 */



haddock_process("process_1");

/***************************************************************************************************
 * EXTERNAL VARIABLES
 */


 
/***************************************************************************************************
 *  GLOBAL FUNCTIONS IMPLEMENTATION
 */
 
/***************************************************************************************************
 * @fn      proc_HostifInit()
 *
 * @brief   init host interface
 *
 * @author	chuanpengl
 *
 * @param   none
 *
 * @return  none
 */
void proc_HostifInit(os_uint8 priority)
{
    struct process *ptProcHostIf = process_create(proc_HostifEntry__, priority);
    g_procHostifPid = ptProcHostIf->_pid;
    hostIf_Init();
    os_ipc_set_signal(this->_pid, PROC_HOSTIF_SIG_PERIOD); // start the set-signal loop
}

/***************************************************************************************************
 * LOCAL FUNCTIONS IMPLEMENTATION
 */
/***************************************************************************************************
 * @fn      proc_HostifEntry__()
 *
 * @brief   host interface process
 *
 * @author	chuanpengl
 *
 * @param   a_nPid  - process id
 *          a_bvSig  - process signal bit vector
 *
 * @return  remain bit vector
 */
signal_bv_t proc_HostifEntry__(os_pid_t a_nPid, signal_bv_t a_bvSig)
{
    signal_bv_t bvSignal = a_bvSig;
    static struct timer *t = NULL;
    
    haddock_assert(a_nPid == this->_pid);
    
    if (a_bvSig & SIGNAL_SYS_MSG) {
        bvSignal = a_bvSig ^ SIGNAL_SYS_MSG;
    }
    
    if (a_bvSig & PROC_HOSTIF_SIG_PERIOD) {
        // jt - set 100ms period for external control resolution
        customize_set_signal(g_procHostifPid, PROC_HOSTIF_SIG_PERIOD, 100);
        
        hostIf_Run();
        

        process_sleep();
        bvSignal = a_bvSig ^ PROC_HOSTIF_SIG_PERIOD;
    }
    

    return bvSignal;
}

 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/
