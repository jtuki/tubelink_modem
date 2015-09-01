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

#include "simple_log.h"
#include "kernel/timer.h"

#if defined SIMPLE_LOG_ENABLE && (SIMPLE_LOG_ENABLE == OS_TRUE)
static const char *log_type_string[4] = {
    " ", "*", "W", "E",
};

/* refer to:
    http://www.velocityreviews.com/forums/t437343-printf-wrapper.html
    http://www.cplusplus.com/reference/cstdio/vprintf/
 */

static char _debug_str[HOSTIF_UART_TX_MSG_MAXLEN];
#endif

void print_log(enum log_type type, const char *log, ...)
{
#if defined SIMPLE_LOG_ENABLE && (SIMPLE_LOG_ENABLE == OS_TRUE)
    static va_list args;
    va_start(args, log);

    os_int8 len = 0;
    os_int8 ret = 0;

    static struct time cur_time;
    haddock_get_time_tick_now(& cur_time);

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

    hostIf_SendToHost(_debug_str, len);

    va_end(args);
#endif
}
