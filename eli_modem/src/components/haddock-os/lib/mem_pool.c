/**
 * mem_pool.c
 *
 * \date   
 * \author jtuki@foxmail.com
 */

#include "assert.h"
#include "hdk_utilities.h"
#include "mem_pool.h"

struct mem_pool_hdr *mem_pool_create(os_uint16 capacity, os_uint16 blk_size)
{
    os_size_t _bv_num = capacity/16 + 1;
    /**< we use uint8 for bv_num as above;
     * i.e. the max value for capacity is 255*16 = 4080. */
    haddock_assert(_bv_num <= ((((os_uint32)1)<<8) - 1));
    
    os_size_t _pool_size = capacity * (blk_size + sizeof(struct mem_pool_blk));
    
    os_size_t offset_bv = sizeof(struct mem_pool_hdr);
    os_size_t offset_bv_num = offset_bv + (_bv_num) * sizeof(os_uint16);
    
    os_size_t _mem_size = offset_bv_num
                       + sizeof (os_uint8) // bv_num
                       + _pool_size;
                       
    struct mem_pool_hdr *mem_pool;
    if ((mem_pool = haddock_malloc(_mem_size)) == NULL)
        return NULL;
    haddock_memset(mem_pool, 0, _mem_size);
    
    mem_pool->blk_size = blk_size;
    mem_pool->capacity = capacity;
    mem_pool->size = 0;
    
    os_uint8 *bv_num = (os_uint8 *) ((char *)mem_pool + offset_bv_num);
    *bv_num = _bv_num;
    
    return mem_pool;
}

void mem_pool_destroy(struct mem_pool_hdr *mem_pool)
{
    // we do nothing here. memory poll are not freed.
}

static inline void *get_pool_buffer(struct mem_pool_hdr *hdr)
{
    return (char*) (hdr->bv) + \
           sizeof(os_uint16)*(hdr->capacity/16 + 1) + sizeof(os_uint8);
}

static inline
void *get_blk_buffer(struct mem_pool_hdr *hdr, os_size_t alloc_id)
{
    haddock_assert(alloc_id < hdr->capacity);
    return (char *)get_pool_buffer(hdr) + \
           alloc_id * (sizeof(struct mem_pool_blk) + hdr->blk_size);
}

static inline
void *get_mem_pool_hdr(struct mem_pool_blk *blk)
{
    os_uint8 *bv_num = (os_uint8*) ((char*)blk - \
            (blk->alloc_id)*(sizeof(struct mem_pool_blk) + blk->blk_size) - sizeof(os_uint8));
    return bv_num - sizeof(os_uint16)*(*bv_num) - sizeof(struct mem_pool_hdr);
}

struct mem_pool_blk *mem_pool_alloc_blk(struct mem_pool_hdr *mem_pool,
                                        os_size_t blk_size)
{
    haddock_assert(blk_size == mem_pool->blk_size);

    os_size_t _bv_num = mem_pool->capacity/16 + 1;
    if (mem_pool->size == mem_pool->capacity)
        return NULL;
    
    os_uint16 *bv = mem_pool->bv;
    
    os_size_t i = 0;
    os_size_t found_bit;
    for (; i < _bv_num; i++) {
        if (bv[i] != 0xFFFF) {
            found_bit = find_any_0_bit_uint16(bv[i]);
            bv[i] |= ((os_uint16)1)<<(15-found_bit);
            break;
        }
    }
    
    if (i == _bv_num)
        return NULL;
    
    os_size_t alloc_id = i*16 + found_bit;
    struct mem_pool_blk *blk = get_blk_buffer(mem_pool, alloc_id);
    
    blk->alloc_id = alloc_id;
    blk->blk_size = blk_size;
    haddock_memset(blk->blk, 0, blk_size);
    
    mem_pool->size += 1;
    
    return blk;
}

void mem_pool_free_blk(struct mem_pool_blk *blk)
{
    haddock_assert(blk);
    
    struct mem_pool_hdr *hdr = get_mem_pool_hdr(blk);
    haddock_assert(blk->blk_size == hdr->blk_size);
    
    (hdr->bv)[blk->alloc_id / 16] ^= ((os_uint16)1)<<(15 - (blk->alloc_id % 16));
    
    haddock_assert(hdr->size > 0);
    hdr->size -= 1;
}
