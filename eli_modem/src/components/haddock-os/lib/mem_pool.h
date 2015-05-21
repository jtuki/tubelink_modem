/**
 * mem_pool.h
 *
 * \date    
 * \author  jtuki@foxmail.com
 */

#ifndef HADDOCK_MEM_POOL_H_
#define HADDOCK_MEM_POOL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include "kernel/hdk_memory.h"
#include "haddock_types.h"

/**
 * The structure of a memory pool is:
 *      struct mem_pool_hdr hdr;
 *      uint16 bv[]; (sizeof(bv) == (bv_num))
 *      uint8  bv_num; (bit-vector number, bv_num == (hdr.capacity/16)+1)
 *      uint8 pool[];  (which contains a series of struct mem_pool_blk(s))
 */

struct mem_pool_hdr {
    os_size_t capacity;
    os_size_t blk_size;
    os_size_t size;
    os_uint16 bv[0];
};

struct mem_pool_blk {
    os_size_t blk_size;
    os_size_t alloc_id;    /**< The block id allocated. range [0, @capacity) */
    os_uint8 blk[0];        /**< The block size is @blk_size. */
};

struct mem_pool_hdr *mem_pool_create(os_uint16 capacity, os_uint16 blk_size);

/**
 * \note memory pool are not freed actually.
 */
void mem_pool_destroy(struct mem_pool_hdr *pool);

struct mem_pool_blk *mem_pool_alloc_blk(struct mem_pool_hdr *mem_pool,
                                        os_size_t blk_size);

void mem_pool_free_blk(struct mem_pool_blk *blk);

#define find_mem_pool_blk(blk_payload) \
    (void *) ((os_uint8 *)(blk_payload) - offsetof(struct mem_pool_blk, blk))

#ifdef __cplusplus
}
#endif

#endif /* HADDOCK_MEM_POOL_H_ */
