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

#define SIMPLE_LOG_ENABLE 1

extern void print_debug_str(const char *log, ...);

#ifdef __cplusplus
}
#endif

#endif /* SIMPLE_LOG_H_ */
