/**
 * simple_log.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef SIMPLE_LOG_H_
#define SIMPLE_LOG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "haddock_types.h"
#include <stdarg.h>

/**
 * \sa log_type_string
 */
enum log_type {
    LOG_INFO = 0,
    LOG_INFO_COOL,   /**< also a normal piece of information, but cooler */
    LOG_WARNING,
    LOG_ERROR,
};

#if defined (MODEM_FOR_GATEWAY) && MODEM_FOR_GATEWAY == OS_TRUE
// disable print_log() for gateway modem
#define SIMPLE_LOG_ENABLE   OS_FALSE
#elif defined (MODEM_FOR_END_DEVICE) && MODEM_FOR_END_DEVICE == OS_TRUE
// enable print_log() for end-device modem
#define SIMPLE_LOG_ENABLE   OS_TRUE
#endif

extern void print_log(enum log_type type, const char *log, ...);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_LOG_H_ */
