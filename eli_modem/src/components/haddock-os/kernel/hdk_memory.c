/**
 * memory.c
 *
 * \date   
 * \author jtuki@foxmail.com
 */

#include "haddock_types.h"
#include "kernel/kernel_config.h"
#include "kernel/hdk_memory.h"
#include "lib/assert.h"
#include "lib/hdk_utilities.h"

static os_uint8 __haddock_memory_allocation[HDK_CFG_MEMORY_FOR_MALLOC];

struct __memory_blk {
    /**
     * 1  bits: used
     * 7  bits: _reserved 
     * 24 bits: blk_len
     */
    os_uint32 hdr;
    os_uint8 blk[0];
};

#define MAX_MEMORY_BLK_SIZE     ((((os_uint32)1)<<24)-1-sizeof(struct __memory_blk))

/**
 * \sa HDK_CFG_MEMORY_FOR_MALLOC
 */
void *haddock_malloc(os_size_t size)
{
    haddock_assert(size < HDK_CFG_MEMORY_FOR_MALLOC
                   && size < MAX_MEMORY_BLK_SIZE);
    
    os_uint32 idx = 0;
    struct __memory_blk *blk = NULL;
    os_boolean found = OS_FALSE;
    
    while (!found) {
        if (idx > HDK_CFG_MEMORY_FOR_MALLOC - (sizeof(struct __memory_blk) + size))
            break;
        blk = (struct __memory_blk *) (& __haddock_memory_allocation[idx]);
        if ((blk->hdr & BV(31)) == BV(31)) {
            // has been used
            idx += sizeof(struct __memory_blk) + (blk->hdr & 0x00FFFFFF);
            continue;
        } else {
            // has not been used
            blk->hdr |= BV(31);
            blk->hdr &= 0xFF000000;
            blk->hdr |= size;
            found = OS_TRUE;
            break;
        }
    }
    
    if (found)
        return blk->blk;
    else
        return NULL;
}

void haddock_free(void *ptr)
{
    haddock_assert(ptr);
    // we actually do not allow memory free currently todo
}

void *haddock_memcpy(void *dest, const void *src, os_size_t len)
{
    haddock_assert(dest && src);
    
    char *_dest = dest;
    const char *_src = src;
    
    for (os_size_t i=0; i < len; i++) {
        _dest[i] = _src[i];
    }
    
    return dest;
}

void *haddock_memset(void *dest, char value, os_size_t len)
{
    haddock_assert(dest);
    
    char *_dest = dest;
    
    for (os_size_t i=0; i < len; i++) {
        _dest[i] = value;
    }
    
    return dest;
}
