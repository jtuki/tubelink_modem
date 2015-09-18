/**
 * simple_log.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "lib/assert.h"
#include "app_hostif.h"
#include <stdio.h>

#include "kernel/timer.h"
#include "lpwan_config.h"

#include "simple_log.h"

#if defined (MODEM_FOR_GATEWAY) && MODEM_FOR_GATEWAY == OS_TRUE
/**< for gateway modems */
#if defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_CPU_CONNECTED
#define SIMPLE_LOG_ENABLE   OS_FALSE
#elif defined (LPWAN_GW_RUNNING_MODE) && LPWAN_GW_RUNNING_MODE == LPWAN_GW_RUNNING_MODE_AUTO_TEST
#define SIMPLE_LOG_ENABLE   OS_TRUE
#endif
#elif defined (MODEM_FOR_END_DEVICE) && MODEM_FOR_END_DEVICE == OS_TRUE
/**< for end device modems */
// enable print_log() for end-device modem
#define SIMPLE_LOG_ENABLE   OS_TRUE
#endif

#if defined SIMPLE_LOG_ENABLE && (SIMPLE_LOG_ENABLE == OS_TRUE)
static const char *log_type_string[5] = {
    " ", "*", "W", "E", "DEBUG_ONLY_NO_PRINT",
};
static char _debug_str[HOSTIF_UART_TX_MSG_MAXLEN];
#endif


#define xPRINT_LOG_DUTY_CYCLE_ENABLED // only for debug purpose

/* refer to:
    http://www.velocityreviews.com/forums/t437343-printf-wrapper.html
    http://www.cplusplus.com/reference/cstdio/vprintf/
 */
void print_log(enum log_type type, const char *log, ...)
{
#if defined SIMPLE_LOG_ENABLE && (SIMPLE_LOG_ENABLE == OS_TRUE)
    static struct time cur_time;
    haddock_get_time_tick_now(& cur_time);

#ifdef PRINT_LOG_DUTY_CYCLE_ENABLED
#define PRINT_LOG_DUTY_CYCLE_TIMES      5
#define PRINT_LOG_DUTY_CYCLE_SECONDS    10  // wait 10 seconds for next print_log()

    static os_uint8 duty_cycle_cnter = 0; /**< allow several  */
    static os_boolean run_first_time = OS_TRUE;
    static struct time prev_time;

    if (run_first_time) {
        haddock_get_time_tick_now(& prev_time);
        run_first_time = OS_FALSE;
    }

    if (++duty_cycle_cnter > PRINT_LOG_DUTY_CYCLE_TIMES) {
        if (cur_time.s <= prev_time.s)
            return;
        else if (cur_time.s - prev_time.s <= PRINT_LOG_DUTY_CYCLE_SECONDS)
            return;
        else {
            duty_cycle_cnter = 0;
            prev_time = cur_time;
        }
    }
#endif

    static va_list args;
    va_start(args, log);

    os_int8 len = 0;
    os_int8 ret = 0;

    ret = (os_int8) sprintf(_debug_str, "%lX.%03ld %s: ",
                                         (unsigned long int) cur_time.s,
                                         (unsigned long int) cur_time.ms,
                                         log_type_string[(int) type]);
    haddock_assert(ret > 0);
    len += ret;

    haddock_assert(len > 0 && len < HOSTIF_UART_TX_MSG_MAXLEN-2);

    ret = (os_int8) vsprintf(_debug_str + len, log, args);
    haddock_assert(ret > 0);
    len += ret;
    haddock_assert(len > 0 && len < HOSTIF_UART_TX_MSG_MAXLEN-2);

    _debug_str[len++] = '\r';
    _debug_str[len++] = '\n';

    if (type < LOG_NO_PRINT) {
        hostIf_SendToHost(_debug_str, len);
    }

    va_end(args);
#endif
}
