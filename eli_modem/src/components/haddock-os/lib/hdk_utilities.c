/**
 * hdk_utilities.c
 *
 * \date   
 * \author jtuki@foxmail.com
 * \author 
 */

#include "assert.h"
#include "hdk_utilities.h"

/*---------------------------------------------------------------------------*/
/**
 * string and bit manipulation.
 * @{
 */

os_boolean is_equal_string(const char *a, const char *b)
{
    os_size_t i = 0;
    for (; a[i] != '\0' && b[i] != '\0'; i++) {
        if (a[i] == b[i])
            continue;
    }
    if (a[i] == b[i] && a[i] == '\0')
        return OS_TRUE;
    else
        return OS_FALSE;
}

/**
 * eg. return 0 for 0x800B, 1 for 0x4030 etc. 
 */
os_size_t find_first_1_bit_uint16(os_uint16 n)
{
    haddock_assert(n != 0);
    os_uint16 mask = 0x8000;
    os_size_t i = 0;
    while ((n & mask) == 0) {
        mask = mask >> 1;
        i++;
    }
    return i;
}

os_size_t find_any_1_bit_uint16(os_uint16 n)
{
    return find_first_1_bit_uint16(n);
}

/**< @} */
/*---------------------------------------------------------------------------*/
/**
 * The random number generation algorithm is borrowed here.
 * http://goo.gl/BsouUB
 * @{
 */

#define RAND_LOCAL_MAX  2147483647 // ((1<<31) - 1)

static os_int32 next = 1;
static os_boolean is_srand = OS_FALSE;

/**
 * Use some random-likely value as seed first.
 * eg. the device's unique ID.
 */
void hdk_srand(os_int32 seed)
{
    next = seed; 
    is_srand = OS_TRUE;
}

os_int32 hdk_rand(void)
{
    haddock_assert(is_srand);
    return ((next = next * 1103515245 + 12345) % RAND_LOCAL_MAX);
}

os_uint32 hdk_randr(os_uint32 min, os_uint32 max)
{
    haddock_assert(is_srand);
    return (os_uint32)hdk_rand() % (max - min + 1) + min;
}

/** @} */
/*---------------------------------------------------------------------------*/

