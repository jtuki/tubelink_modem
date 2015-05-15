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

#define __should_never_fall_here() haddock_assert(OS_FALSE)

#if defined HDK_USER_CFG_MAIN_PC_TEST && HDK_USER_CFG_MAIN_PC_TEST == OS_TRUE
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
#define haddock_assert(condition) do { \
    if (!(condition)) \
        while (1) {} \
} while (0)
#else
#define haddock_assert(condition)
#endif

#ifdef __cplusplus
}
#endif

#endif /* ASSERT_H_ */
