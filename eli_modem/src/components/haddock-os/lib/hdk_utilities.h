/**
 * string_utils.h
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */
#ifndef HDK_UTILITIES_H_
#define HDK_UTILITIES_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "haddock_types.h"

/**
 * @example
 *      BV(0) - 0x0001
 *      BV(3) - 0x1000
 */
#define BV(n)  (((os_uint32)1)<<(n))

os_boolean is_equal_string(const char *a, const char *b);

os_size_t find_first_1_bit_uint16(os_uint16 n);
os_size_t find_any_1_bit_uint16(os_uint16 n);
#define find_first_0_bit_uint16(n)  find_first_1_bit_uint16(~(n))
#define find_any_0_bit_uint16(n)    find_any_1_bit_uint16(~(n))

void hdk_srand(os_int32 seed);
os_int32 hdk_rand(void);
os_uint32 hdk_randr(os_uint32 min, os_uint32 max);

#ifndef offsetof
#define offsetof(st, m) ((os_size_t) (& ((st *)0)->m))
#endif

#ifdef __cplusplus
}
#endif

#endif /* HDK_UTILITIES_H_ */
