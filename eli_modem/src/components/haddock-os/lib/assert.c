/**
 * assert.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "assert.h"

#if defined HDK_CFG_DEBUG && (HDK_CFG_DEBUG == OS_TRUE)
os_uint32   __haddock_call_function_line;
const char *__haddock_call_function_file;
#endif
