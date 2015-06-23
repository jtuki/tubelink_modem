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

#define SIMPLE_LOG_ENABLE 1

extern void print_log(enum log_type type, const char *log, ...);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_LOG_H_ */
