/***************************************************************************************************
* @file name	easylinkin_modem_process.h
* @data   		2015/04/27
* @auther   	chuanpengl
* @module   	endnode processes
* @brief 		endnode processes, include host if process,
***************************************************************************************************/
#ifndef __ELI_ENDNODE_PROCESS_H__
#define __ELI_ENDNODE_PROCESS_H__
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
/* process priority define */
#define PROC_HOSTIF_PRIORITY         (1)

    
    
/* process signal define */
#define PROC_HOSTIF_SIG_PERIOD        (BV(0))
    
    
    
    
    
#define customize_set_signal(dest_pid, signal, period_ms) \
    if (t == NULL) { \
        t = os_timer_create((dest_pid), (signal), period_ms); \
        haddock_assert(t); \
    } else { \
        os_timer_reconfig(t, (dest_pid), (signal), period_ms); \
    } \
    os_timer_start(t);

/***************************************************************************************************
 * TYPEDEFS
 */


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
void os_processes_init_easylinkin_modem(void);

#ifdef __cplusplus
}
#endif

#endif /* __ELI_ENDNODE_PROCESS_H__ */
 
/***************************************************************************************************
* HISTORY LIST
* 1. Create File by author @ data
*  	context: here write modified history
*
***************************************************************************************************/