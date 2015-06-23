/**
 * assert.h
 *
 * \date   
 * \author jtuki@foxmail.com
 */
#ifndef HADDOCK_ASSERT_H_
#define HADDOCK_ASSERT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel/kernel_config.h"
#include "hdk_user_config.h"
#include "app_hostif.h"
#include "simple_log.h"

#define S(x) #x
#define S_(x) S(x)
#define S_LINE__ S_(__LINE__)

#define __should_never_fall_here() haddock_assert(OS_FALSE)

#if defined HDK_USER_CFG_HAL_PC && HDK_USER_CFG_HAL_PC == OS_TRUE
#include <stdio.h>
#include <stdlib.h>
#define haddock_assert(condition) do { \
    if (!(condition)) { \
        printf("*** assert failed - FILE %s LINE %d \n  %s", \
               __FILE__, __LINE__, #condition); \
        exit(-1); \
    } \
} while (0)
#elif defined HDK_CFG_DEBUG && HDK_CFG_DEBUG == OS_TRUE
// not on PC platform, debug mode enabled (simply use watchdog to implement assert).
#define haddock_assert(condition) do { \
    if (!(condition)) { \
        print_log(LOG_ERROR, "assert fail: %d", __LINE__); \
        while (1) {} \
    } \
} while (0)

extern os_uint32   __haddock_call_function_line;
extern const char *__haddock_call_function_file;
#else
#define haddock_assert(condition)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ASSERT_H_ */
