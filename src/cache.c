/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:13:56
 * @FilePath    : /hmsys/src/cache.c
 * @Description : 缓存逻辑的实现
 */

#include "cache.h"
#include "log.h"
#include "cache_layer.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "driver.h"
#include "global.h"
#include "config.h"

/**
 * @brief 设置缓存设备的默认参数
 * 
 * @param cache 缓存设备指针
 */
void cache_set_default_params(struct cache_dev *cache) {
    // 从配置文件中获取参数
    get_cache_config(cache);
    
    // 计算set数量和缓存块数量
    cache->set_numbers = cache->size / (cache->block_size * cache->assoc);
    cache->cache_numbers = cache->size / cache->block_size;
    
    // 如果配置文件中没有设置懒更新参数，设置默认值
    // if (cache->lazy_update != 0 && cache->lazy_update != 1) {
    //     cache->lazy_update = 0; // 默认不采用懒更新
    // }
}

/**
 * @brief 为缓存设备分配内存
 * 
 * @param cache 缓存设备指针
 * @return int 成功返回0，失败返回-1
 */
int cache_allocate_memory(struct cache_dev *cache) {
    // 分配缓存块数组
    cache->caches = (struct cache_block *)malloc(cache->cache_numbers * sizeof(struct cache_block));
    if (!cache->caches) {
        LOG(ERROR, "Failed to allocate memory for cache blocks\n");
        return -1;
    }
    
    // 分配缓存set数组
    cache->cache_sets = (struct cache_set *)malloc(cache->set_numbers * sizeof(struct cache_set));
    if (!cache->cache_sets) {
        LOG(ERROR, "Failed to allocate memory for cache sets\n");
        free(cache->caches);
        return -1;
    }
    
    return 0;
}

/**
 * @brief 释放缓存设备的内存
 * 
 * @param cache 缓存设备指针
 */
void cache_free_memory(struct cache_dev *cache) {
    if (cache) {
        if (cache->cache_sets) {
            for (int i = 0; i < cache->set_numbers; i++) {
                if (cache->cache_sets[i].hash_buckets) {
                    free(cache->cache_sets[i].hash_buckets);
                }
            }
            free(cache->cache_sets);
        }
        
        if (cache->caches) {
            free(cache->caches);
        }
        
        free(cache);
    }
}

/**
 * @brief 创建并初始化缓存设备
 * 
 * @return struct cache_dev* 成功返回缓存设备指针，失败返回NULL
 */
struct cache_dev* cache_create_and_init() {
    // 创建一个缓存设备
    struct cache_dev *cache = (struct cache_dev *)malloc(sizeof(struct cache_dev));
    if (!cache) {
        LOG(ERROR, "Failed to allocate memory for cache device\n");
        return NULL;
    }
    
    // 设置缓存参数
    cache_set_default_params(cache);
    
    // 分配内存
    if (cache_allocate_memory(cache) != 0) {
        free(cache);
        return NULL;
    }
    
    // 初始化缓存设备
    cache_initialize(cache);
    
    LOG(INFO, "Cache initialized with %d sets, %d blocks, %d-way associative\n", 
        cache->set_numbers, cache->cache_numbers, cache->assoc);
    
    return cache;
}

/**
 * @brief 在 cache_set 中查询 blkno 是否被缓存过
 *
 * @param cache
 * @param cache_set
 * @param blkno
 * @return int*
 */
static long *cache_get_hash_bucket(struct cache_dev *cache, struct cache_set *cache_set, long blkno) {
    // unsigned int hash = blkno;

    // 这里确保 offset 是按照 cache_block_size 对齐

    /*
        这里的哈希桶的理解如下：先按照直接映射，根据请求的块号得到 set 中对应的 cache_block 号，
        如果 hash 桶中没有值，那就将该缓存块分配给这个请求(返回值是 0xFF)，如果有值，那么就返回
        之前已经缓存了的 cache block 号。
    */
    unsigned long cache_block_num = blkno / cache->block_size;
    unsigned long hash_index = cache_block_num % cache->assoc;
    // 利用哈希桶，直接查找对应的 blkno 之前是否被缓存过，这样就不需要循环遍历了
    // return &(cache_set->hash_buckets[blkno % 512]);
    return &(cache_set->hash_buckets[hash_index]);
}

/**
 * @brief 给定 set 号和 blkno 查找对应的 block 是否被缓存过
 *
 * @param cache
 * @param set 要查找的 set 号
 * @param blkno 要查找的 block num
 * @return 如果被缓存过(命中)则返回对应的 cache block 号，不命中返回 -1
 */
static long cache_hash_lookup(struct cache_dev *cache, int set, long blkno) {
    struct cache_set *cache_set = &cache->cache_sets[set];
    long index;
    struct cache_block *cacheblk;
    long set_ix;

    set_ix = *cache_get_hash_bucket(cache, cache_set, blkno);

    /*
        这里查找 blkno 是否被缓存过的策略是，先根据 blkno 号找到对应的 set，
        按照直接映射，查找对应的 hash 桶中是否有值，若有值判断请求的块号是否一致，
        若一致则表示命中，返回 cache block 号，不一致则通过 hash 冲突的链式地址法查找。
    */
    while (set_ix != CACHE_LRU_NULL) {
        // index = set * cache->assoc + set_ix;
        index = set_ix;
        cacheblk = &cache->caches[index];

        // 请求的 blkno 和 cache 块中的 blkno 比较，相等表示命中，返回 cache block 号
        if (blkno / cache->block_size * cache->block_size == cacheblk->blkno) {
        // if (blkno == cacheblk->blkno) {
            return index;
        }
        set_ix = cacheblk->hash_next;
    }
    return -1;
}

/**
 * @brief 查找 blkno 是否命中，若命中则将 index 设为 cache block 号
 *
 * @param cache
 * @param blkno         请求的块号
 * @param start_index   请求的块号对应的组相联的组(set)的首地址
 * @param index         要返回的有效块号(全局的 cache block number)
 */
static void find_valid_blkno(struct cache_dev *cache, long blkno, int set_number, long *index) {
    *index = cache_hash_lookup(cache, set_number, blkno);
    // index = -1 表示不命中
    if (*index == -1) {
        return;
    }

#ifdef DEBUG
    int cache_block_num = set_number * cache->assoc + *index;
    struct cache_block *cacheblk = &(cache->caches[cache_block_num]);
    LOG(DEBUG, "block_state(1: INVALID, 2: VALID, 64: DIRTY, 66: VALID & DIRTY): %d, lru_state(1: Hot, 2: Warm): %d\n", cacheblk->cache_state, cacheblk->lru_state);
    // LOG(DEBUG, "curr invalid_head: %d, hash_prev: %d, hash_next: %d\n", cache_set->invalid_head, cacheblk->hash_prev, cacheblk->hash_next);
#endif

    // // 命中之后，需要根据不同的缓存策略调整命中的缓存块的位置(即热度)
    // switch (cache->cache_policy) {
    //     case CACHE_LRU:
    //         cache_lru_accessed(cache, *index);
    //         break;
    //     default:
    //         break;
    // }
}

// 没问题
static void cache_hash_remove(struct cache_dev *cache, long index) {
    struct cache_set *cache_set;
    struct cache_block *cacheblk;
    long *hash_bucket;

    long start_index, set;

    if (index == -1)
        return;
    set = index / cache->assoc;
    cache_set = &cache->cache_sets[set];
    cacheblk = &cache->caches[index];
    start_index = set * cache->assoc;
    hash_bucket = cache_get_hash_bucket(cache, cache_set, cacheblk->blkno);

    if (cacheblk->hash_prev != CACHE_LRU_NULL) {
        // cache->caches[start_index + cacheblk->hash_prev].hash_next = cacheblk->hash_next;
        cache->caches[cacheblk->hash_prev].hash_next = cacheblk->hash_next;
    } else {
        *hash_bucket = cacheblk->hash_next;
    }
    if (cacheblk->hash_next != CACHE_LRU_NULL) {
        // cache->caches[start_index + cacheblk->hash_next].hash_prev = cacheblk->hash_prev;
        cache->caches[cacheblk->hash_next].hash_prev = cacheblk->hash_prev;
    }
    cacheblk->hash_prev = CACHE_LRU_NULL;
    cacheblk->hash_next = CACHE_LRU_NULL;
}

/**
 * @brief 新的 cache block 往 hash 链表的头部添加，为双向不循环链表
 *
 * @param cache
 * @param index
 */
static void cache_invalid_insert(struct cache_dev *cache, int index) {
    struct cache_set *cache_set;
    struct cache_block *cacheblk;

    int set = index / cache->assoc;

    int start_index = set * cache->assoc;

    int set_ix = index % cache->assoc;

    /* index validity checks */

    cacheblk = &cache->caches[index];
    /* It has to be an INVALID block */
    /* It cannot be on the per-set hash */
    /* Insert this block at the head of the invalid list */
    cache_set = &cache->cache_sets[set];

    cacheblk->hash_next = cache_set->invalid_head;

    if (cache_set->invalid_head != CACHE_LRU_NULL) {
        cache->caches[start_index + cache_set->invalid_head].hash_prev = set_ix;
    }
    cache_set->invalid_head = set_ix;
}

/**
 * @brief
 *
 * @param cache
 * @param start_set
 * @param io_start
 */
static void cache_inval_block_set(struct cache_dev *cache, int start_set, long io_start) {
    int index;
    struct cache_block *cacheblk;

    index = cache_hash_lookup(cache, start_set, io_start);

    if (index == -1) {
        return;
    }
    cacheblk = &cache->caches[index];

    // 如果该缓存块不是脏块，则将该块从 hash 桶中移出，并设置该块为 invalid，插入 invalid 队列
    if (!(cacheblk->cache_state & DIRTY)) {
        // printf("not dirty\n");
        cache_hash_remove(cache, index);
        cacheblk->cache_state = INVALID;
        cache_invalid_insert(cache, index);
        return;
    }

    // 如果是脏块，则设置为 valid，表明脏块已经被刷到磁盘上，数据一致
    if (cacheblk->cache_state & DIRTY) {
        cacheblk->cache_state = VALID;
    }
    return;
}

/**
 * @brief 根据请求的块号找到对应的 set 号
 *
 * @param cache
 * @param blkno
 * @return int
 */
inline static int hash_block(struct cache_dev *cache, long blkno) {
    // 一个set有多少个cache块，一个块有多少个扇区
    // return (blkno / (cache->assoc * cache->block_size)) % cache->set_numbers; #lzq
    return (blkno / cache->block_size) % cache->set_numbers;
}

/**
 * @brief 请求的块号按照 block_size 对齐(默认是 4KB)
 *
 * @param cache
 * @param blkno
 * @param bcount
 */
static void cache_inval_blocks(struct cache_dev *cache, long blkno, int bcount) {
    long io_start = blkno / cache->block_size * cache->block_size;
    int start_set = hash_block(cache, io_start);
    // 4KB 粒度对齐。
    cache_inval_block_set(cache, start_set, io_start);
}

/**
 * @brief 将 cache block 逐个添加到 hot 和 warm 链表的 LRU 端，即 head 端
 *        hot 和 warm 链表的 tail 端是 MRU 端，head 是 LRU 端
 *
 * @param cache
 * @param index
 */
static void cache_reclaim_add_block_to_list_lru(struct cache_dev *cache, int index) {
    int set = index / cache->assoc;
    int start_index = set * cache->assoc;
    int my_index = index - start_index;

    struct cache_block *cacheblk = &cache->caches[index];
    struct cache_set *cache_set = &cache->cache_sets[set];

    cacheblk->lru_prev = CACHE_LRU_NULL;

    // 添加到LRU的头部， LRU端  ---  MRU端
    if (cacheblk->lru_state & LRU_WARM) {
        cacheblk->lru_next = cache_set->warmlist_lru_head;
        if (cache_set->warmlist_lru_head == CACHE_LRU_NULL) {
            cache_set->warmlist_lru_tail = my_index;
        } else {
            cache->caches[cache_set->warmlist_lru_head + start_index].lru_prev = my_index;
        }
        cache_set->warmlist_lru_head = my_index;
    } else {
        cacheblk->lru_next = cache_set->hotlist_lru_head;
        if (cache_set->hotlist_lru_head == CACHE_LRU_NULL) {
            cache_set->hotlist_lru_tail = my_index;
        } else {
            cache->caches[cache_set->hotlist_lru_head + start_index].lru_prev = my_index;
        }
        cache_set->hotlist_lru_head = my_index;
    }

    if (cacheblk->lru_state & LRU_WARM) {
        cache_set->lru_warm_blocks++;
        cache->lru_warm_blocks++;
    } else {
        cache_set->lru_hot_blocks++;
        cache->lru_hot_blocks++;
    }
}

// 删除
static void cache_reclaim_remove_block_from_list(struct cache_dev *cache, int index) {
    int set = index / cache->assoc;
    int start_index = set * cache->assoc;

    struct cache_block *cacheblk = &cache->caches[index];
    struct cache_set *cache_set = &cache->cache_sets[set];

    if ((cacheblk->lru_prev == CACHE_LRU_NULL) && (cacheblk->lru_next == CACHE_LRU_NULL)) {
        /*
         * Is this the only member on the list ? Or is this not on the list
         * at all ?
         */
        if (cacheblk->lru_state & LRU_WARM) {
            if (cache_set->warmlist_lru_head == CACHE_LRU_NULL && cache_set->warmlist_lru_tail == CACHE_LRU_NULL)
                return;
        } else {
            if (cache_set->hotlist_lru_head == CACHE_LRU_NULL && cache_set->hotlist_lru_tail == CACHE_LRU_NULL)
                return;
        }
    }

    if (cacheblk->lru_prev != CACHE_LRU_NULL) {
        cache->caches[cacheblk->lru_prev + start_index].lru_next = cacheblk->lru_next;
    } else {
        if (cacheblk->lru_state & LRU_WARM) {
            cache_set->warmlist_lru_head = cacheblk->lru_next;
        } else {
            cache_set->hotlist_lru_head = cacheblk->lru_next;
        }
    }

    if (cacheblk->lru_next != CACHE_LRU_NULL) {
        cache->caches[cacheblk->lru_next + start_index].lru_prev = cacheblk->lru_prev;
    } else {
        if (cacheblk->lru_state & LRU_WARM) {
            cache_set->warmlist_lru_tail = cacheblk->lru_prev;
        } else {
            cache_set->hotlist_lru_tail = cacheblk->lru_prev;
        }
    }
    if (cacheblk->lru_state & LRU_WARM) {
        cache->lru_warm_blocks--;
        cache_set->lru_warm_blocks--;
        if (cache_set->lru_warm_blocks == 0) {
        }
    } else {
        cache->lru_hot_blocks--;
        cache_set->lru_hot_blocks--;
        if (cache_set->lru_hot_blocks == 0) {
        }
    }
}

// no problem
static void cache_reclaim_add_block_to_list_mru(struct cache_dev *cache, int index) {
    int set = index / cache->assoc;
    int start_index = set * cache->assoc;

    int my_index = index - start_index;
    struct cache_block *cacheblk = &cache->caches[index];
    struct cache_set *cache_set = &cache->cache_sets[set];

    cacheblk->lru_next = CACHE_LRU_NULL;

    if (cacheblk->lru_state & LRU_WARM) {
        // 如果状态是在温链表
        cacheblk->lru_prev = cache_set->warmlist_lru_tail;
        if (cache_set->warmlist_lru_tail == CACHE_LRU_NULL) {
            cache_set->warmlist_lru_head = my_index;
        } else {
            cache->caches[cache_set->warmlist_lru_tail + start_index].lru_next = my_index;
        }
        cache_set->warmlist_lru_tail = my_index;

    } else {
        cacheblk->lru_prev = cache_set->hotlist_lru_tail;
        if (cache_set->hotlist_lru_tail == CACHE_LRU_NULL) {
            cache_set->hotlist_lru_head = my_index;
        } else {
            cache->caches[cache_set->hotlist_lru_tail + start_index].lru_next = my_index;
        }
        cache_set->hotlist_lru_tail = my_index;
    }
    // 加上去更新统计信息

    if (cacheblk->lru_state & LRU_WARM) {
        cache->lru_warm_blocks++;
        cache_set->lru_warm_blocks++;
    } else {
        cache->lru_hot_blocks++;
        cache_set->lru_hot_blocks++;
    }
}

/**
 * @brief 将 cache block 移动到链表的 MRU 端，即 tail 端
 *        步骤是先将 cache block 从链表中移除，然后再添加到 tail 端
 *
 * @param cache
 * @param index
 */
static void cache_reclaim_move_to_mru(struct cache_dev *cache, int index) {
    struct cache_block *cacheblk = &cache->caches[index];

    cache_reclaim_remove_block_from_list(cache, index);
    /* And add it to LRU Tail (MRU side) of its list */
    cache_reclaim_add_block_to_list_mru(cache, index);
}

/**
 * @brief 提升索引
 *
 * @param cache
 * @param index
 * @return int
 */
static int cache_reclaim_promote_block(struct cache_dev *cache, int index) {
    struct cache_block *cacheblk = &cache->caches[index];

    int hot_block;
    int set = index / cache->assoc;
    int start_index = set * cache->assoc;

    struct cache_set *cache_set = &cache->cache_sets[set];

    hot_block = cache_set->hotlist_lru_head;
    // hot_block = cache_set->hotlist_lru_tail;

    if (hot_block == CACHE_LRU_NULL)
        /* We cannot swap this block into the hot list */
        return 0;

    hot_block += start_index;

    /* Remove warm block from its list first */
    cache_reclaim_remove_block_from_list(cache, index);

    /* Remove hot block identified above from its list */
    cache_reclaim_remove_block_from_list(cache, hot_block);

    /* Swap the 2 blocks */

    cacheblk->lru_state &= ~LRU_WARM;
    cacheblk->lru_state |= LRU_HOT;
    // 加入hot链表
    cache_reclaim_add_block_to_list_lru(cache, index);

    cacheblk = &cache->caches[hot_block];

    cacheblk->lru_state &= ~LRU_HOT;
    cacheblk->lru_state |= LRU_WARM;
    cache_reclaim_add_block_to_list_mru(cache, hot_block);
    return 1;
}

/**
 * @brief invalid 链表是一个双向逻辑链表，hash_next 和 hash_prev 指针
 *        实际存储的是数组下标，且下标是从大到小排列。从大到小方向为 hash_next
 *        从小到大方向为 hash_prev。
 *        作用是将当前 cache block 结点从 invalid 链表中移除，将前后指针全部置空。
 *
 * @param cache
 * @param index
 */
static void cache_invalid_remove(struct cache_dev *cache, int index) {
    struct cache_set *cache_set;
    struct cache_block *cacheblk;
    int start_index, set;

    cacheblk = &cache->caches[index];
    set = index / cache->assoc;
    start_index = set * cache->assoc;
    cache_set = &cache->cache_sets[set];

    /*
        0 -> 1 -> 2 -> 3
        -> prev
        <- next
    */
    // 移除一个结点后修改链表前后指针
    if (cacheblk->hash_prev != CACHE_LRU_NULL) {
        cache->caches[start_index + cacheblk->hash_prev].hash_next = cacheblk->hash_next;
    } else {
        cache_set->invalid_head = cacheblk->hash_next;
    }
    if (cacheblk->hash_next != CACHE_LRU_NULL) {
        cache->caches[start_index + cacheblk->hash_next].hash_prev = cacheblk->hash_prev;
    }
    cacheblk->hash_prev = CACHE_LRU_NULL;
    cacheblk->hash_next = CACHE_LRU_NULL;
}

// 没问题
static int cache_reclaim_demote_block(struct cache_dev *cache, int index) {
    struct cache_block *cacheblk = &cache->caches[index];
    int warm_block;
    int set = index / cache->assoc;

    struct cache_set *cache_set = &cache->cache_sets[set];

    int start_index = set * cache->assoc;

    warm_block = cache_set->warmlist_lru_tail;

    if (warm_block == CACHE_LRU_NULL) {
        return 0;
    }

    warm_block += start_index;
    /* Remove hot block from its list first */
    cache_reclaim_remove_block_from_list(cache, index);
    /* Remove warm block identified above from its list */
    cache_reclaim_remove_block_from_list(cache, warm_block);
    /* Swap the 2 blocks */

    cacheblk->lru_state &= ~LRU_HOT;
    cacheblk->lru_state |= LRU_WARM;
    cache_reclaim_add_block_to_list_mru(cache, index);

    cacheblk = &cache->caches[warm_block];
    cacheblk->lru_state &= ~LRU_WARM;
    cacheblk->lru_state |= LRU_HOT;
    cache_reclaim_add_block_to_list_lru(cache, warm_block);
    return 1;
}

static int cnt = 0;
// 最后一个更新函数:清理脏块
static void cache_clean_set(struct cache_dev *cache, int set) {
    int to_clean = 0;  // 要下刷的脏块数量
    int start_index = set * cache->assoc;
    int nr_dirty = 0;
    int end_index = start_index + cache->assoc;
    int i = 0;
    int nr_writes = 0;
    struct cache_set *cache_set = &cache->cache_sets[set];
    struct cache_block *cacheblk;
    int scanned = 0;
    // 计算脏块数量，如果超过阈值，则进行下刷
    for (i = start_index; i < end_index; i++) {
        if ((cache->caches[i].cache_state & DIRTY)) {
            nr_dirty++;
        }
    }

    if (nr_dirty < cache->dirty_thresh_set) {
        return;
    }

    to_clean = nr_dirty - cache->dirty_thresh_set;

    // if (nr_dirty > 0)
    //     printf("dirty = %d, to_clean = %d, thresh = %d\n", nr_dirty, to_clean,cache->dirty_thresh_set);

    if (cache->cache_policy == CACHE_FIFO) {
        i = cache_set->set_clean_next;
        while (scanned < cache->assoc && nr_writes < to_clean && nr_writes < cache->max_clean_ios_set) {
            cacheblk = &cache->caches[i];
            if (cacheblk->cache_state & DIRTY) {
                cacheblk->cache_state &= ~DIRTY;
                nr_writes++;
            }
            scanned++;
            i++;
            if (i == end_index) {
                i = start_index;
            }
        }
        cache_set->set_clean_next = i;
    } else {
        int lru_rel_index;
        int iter = 0;
        for (iter = 0; iter < 2; iter++) {
            if (iter == 0) {
                lru_rel_index = cache_set->warmlist_lru_head;
            } else {
                lru_rel_index = cache_set->hotlist_lru_head;
            }
            while (lru_rel_index != CACHE_LRU_NULL && nr_writes < to_clean && nr_writes < cache->max_clean_ios_set) {
                cacheblk = &cache->caches[lru_rel_index + start_index];
                if (cacheblk->cache_state & DIRTY) {
                    // 触发回写
                    cacheblk->cache_state &= ~DIRTY;
                }
                lru_rel_index = cacheblk->lru_next;
                nr_writes++;
                // cnt++;
                // printf("nr_writes = %d, cnt = %d\n", nr_writes, cnt);
            }
        }
    }
    return;
}

// 没问题
static void cache_hash_insert(struct cache_dev *cache, int index) {
    struct cache_set *cache_set = &cache->cache_sets[index / cache->assoc];
    struct cache_block *cacheblk;

    long *hash_bucket;
    // int set_ix = index % cache->assoc;
    int set_ix = index;

    int start_index = (index / cache->assoc) * cache->assoc;

    cacheblk = &cache->caches[index];

    hash_bucket = cache_get_hash_bucket(cache, cache_set, cacheblk->blkno);

    cacheblk->hash_prev = CACHE_LRU_NULL;

    cacheblk->hash_next = *hash_bucket;

    if (*hash_bucket != CACHE_LRU_NULL) {
        // cache->caches[start_index + *hash_bucket].hash_prev = set_ix;
        cache->caches[*hash_bucket].hash_prev = set_ix;
    }
    *hash_bucket = set_ix;
}

/**
 * @brief 采用 LRU 替换策略时，新来一个请求时，将缓存块移动到链表的 MRU 端
 *
 * @param cache
 * @param index
 */
static void cache_lru_accessed(struct cache_dev *cache, int index) {
    cache_reclaim_move_to_mru(cache, index);
    return;
}

/**
 * @brief 采用双链 LRU 替换策略时，新来一个请求时，提升缓存块热度
 *
 * @param cache
 * @param index cache block 号
 */
static void cache_double_linklist_lru_accessed(struct cache_dev *cache, int index) {
    struct cache_block *cacheblk = &cache->caches[index];

    /*
        如果 Hot 链表没满，则加到 Hot 链表的 MRU 端，如果满了，则需要进行替换
        替换的掉的块加入 Warm 链表的 MRU 端

        缓存命中时，如果命中的缓存块在 Hot 链表上，那么直接将其移动到 MRU 端(提升热度)
    */
    if (cacheblk->lru_state & LRU_HOT) {
        cache_reclaim_move_to_mru(cache, index);
        return;
    } else {
        /*
            进入该分支有两种情况，一种是 Hot 已经被分完了，那么应该从 Hot 中替换出一块，
            替换出的那块进入 Warm 的 MRU 端(tail)，具体代码的逻辑是：

            另一种情况是缓存命中，这个时候命中的是 Warm 链表上的缓存块，这个时候直接
            加入到 Hot 链表的 MRU 端
        */
        if (cacheblk->cache_state == INVALID) {
            cache_reclaim_move_to_mru(cache, index);
            return;
        }
    }
    // 提升索引，到hot链表中，BUG
    // if (!cache_reclaim_promote_block(cache, index)) {
    //     /* Could not promote block, move it to mru on warm list */
    //     cache_reclaim_move_to_mru(cache, index);
    // }
}

/**
 * @brief FIFO 缓存替换策略
 *
 * @param cache
 * @param start_index
 * @param index
 */
static void cache_reclaim_fifo_get_old_block(struct cache_dev *cache, long start_index, long *index) {
    int set = start_index / cache->assoc;
    struct cache_set *cache_set = &cache->cache_sets[set];
    int end_index = start_index + cache->assoc;

    int slots_searched = 0;
    int i;

    i = cache_set->set_fifo_next;

    while (slots_searched < cache->assoc) {
        if (cache->caches[i].cache_state == VALID) {
            *index = i;
            break;
        }
        slots_searched++;
        i++;
        if (i == end_index) {
            i = start_index;
        }
    }
    i++;
    if (i == end_index) {
        i = start_index;
    }
    cache_set->set_fifo_next = i;
}

/**
 * @brief 单链 LRU 缓存替换策略，替换的块可以是 VALID 或 DIRTY
 *
 * @param cache
 * @param start_index 当前 set 开始的缓存块号
 * @param oldest_clean 要淘汰的缓存块号
 */
static void cache_reclaim_lru_get_old_block(struct cache_dev *cache, long start_index, long *oldest_clean) {
    struct cache_block *cacheblk;
    int set = start_index / cache->assoc;
    struct cache_set *cache_set = &(cache->cache_sets[set]);
    // tmp 指针为链表的 head 端，即 LRU 端，是需要淘汰的缓存块
    int tmp = cache_set->hotlist_lru_head;

    int is_tape = 1;
    if (is_tape == 0) {
        if (tmp != CACHE_LRU_NULL) {
            *oldest_clean = tmp + start_index;
            // cache_reclaim_move_to_mru(cache, *oldest_clean);
        }
    } else {
        while (tmp != CACHE_LRU_NULL) {
            *oldest_clean = tmp + start_index;
            cacheblk = &cache->caches[*oldest_clean];
            if (!(cacheblk->cache_state & DIRTY)) {
                break;
            }
            if ((cacheblk->cache_state & DIRTY) && (cache->tape_eod == cacheblk->blkno)) {
                cache->tape_eod += cache->block_size;
                break;
            }
            tmp = cacheblk->lru_next;
        }
    }
    return;
}

/**
 * @brief 双链 LRU 缓存替换策略
 *
 * @param cache
 * @param start_index
 * @param index
 */
static void cache_reclaim_double_linklist_lru_get_old_block(struct cache_dev *cache, int start_index, int *index) {
    int lru_rel_index;

    struct cache_block *cacheblk;

    int set = start_index / cache->assoc;

    struct cache_set *cache_set = &cache->cache_sets[set];

    *index = -1;

    lru_rel_index = cache_set->warmlist_lru_head;

    while (lru_rel_index != CACHE_LRU_NULL) {
        cacheblk = &cache->caches[lru_rel_index + start_index];
        // 回收一个cache块，状态是 VALID
        if (cacheblk->cache_state == VALID) {
            *index = cacheblk - &cache->caches[0];
            cache_reclaim_move_to_mru(cache, *index);
            break;
        }
        lru_rel_index = cacheblk->lru_next;
    }

    // 如果 warm 链表上全是 DIRTY，那么从 hot 链表上淘汰
    if (*index != -1)
        return;
    /*
     * We did not find a block on the "warm" LRU list that we could take, pick
     * a block from the "hot" LRU list.
     */

    lru_rel_index = cache_set->hotlist_lru_head;
    while (lru_rel_index != CACHE_LRU_NULL) {
        cacheblk = &cache->caches[lru_rel_index + start_index];
        // 这里改一下
        if (cacheblk->cache_state == VALID) {
            *index = cacheblk - &cache->caches[0];
            /*
             * Swap this block with the MRU block in the warm list.
             * To maintain equilibrium between the lists
             * 1) We put this block in the MRU position on the warm list
             * 2) Remove the block in the LRU position on the warm list and
             * 3) Move that block to the LRU position on the hot list.
             */
            if (!cache_reclaim_demote_block(cache, *index)) {
                /*
                 * We cannot demote this block to the warm list
                 * just move it to the MRU position.
                 */
                cache_reclaim_move_to_mru(cache, *index);
            }
            break;
        }
        lru_rel_index = cacheblk->lru_next;
    }
}

/**
 * @brief 缓存未命中，查找空闲块(invalid)
 *
 * @param cache
 * @param set
 * @return int
 */
static int find_invalid_blkno(struct cache_dev *cache, int set) {
    struct cache_set *cache_set;
    int index;
    struct cache_block *cacheblk;

    cache_set = &cache->cache_sets[set];

    index = cache_set->invalid_head;

    if (index == CACHE_LRU_NULL)
        return -1;

    index += (set * cache->assoc);

    cacheblk = &cache->caches[index];

#ifdef DEBUG
    LOG(DEBUG, "block_state(1: INVALID, 2: VALID, 64: DIRTY, 66: VALID & DIRTY): %d, lru_state(1: Hot, 2: Warm): %d\n", cacheblk->cache_state, cacheblk->lru_state);
    LOG(DEBUG, "curr invalid_head: %ld, hash_prev: %ld, hash_next: %ld\n", cache_set->invalid_head, cacheblk->hash_prev, cacheblk->hash_next);
#endif

    // 从空闲块链表中取出当前的 cache 块
    cache_invalid_remove(cache, index);

    if (index != -1) {
        // #ifdef DEBUG
        // LOG(DEBUG, "After remove from invalid, hash_prev: %d, hash_next: %d\n", cacheblk->hash_prev, cacheblk->hash_next);
        // #endif
    }
    return index;
}

/**
 * @brief 缓存替换策略，可以自定义自己的替换策略
 *
 * @param cache
 * @param start_index
 * @param oldest_clean
 */
static void find_reclaim_blkno(struct cache_dev *cache, long start_index, long *oldest_clean) {
#ifdef DEBUG
    LOG(DEBUG, LOG_DEBUG_STYLE "Cache eviction" LOG_STYLE_CLEAR "\n");
#endif

    // 在这里可以添加自定义的缓存替换策略
    switch (cache->cache_policy) {
        case CACHE_FIFO:
            cache_reclaim_fifo_get_old_block(cache, start_index, oldest_clean);
            break;
        case CACHE_LRU:
            cache_reclaim_lru_get_old_block(cache, start_index, oldest_clean);
            break;
        default:
            break;
    }

        /*
            替换下来的块如果是脏块，才需要写回，这里逻辑上没有计算下刷到磁盘上的时间，
            只是将 DIRTY 设置成 VALID
        */
#if 0  // lzq
    if (cache->caches[*oldest_clean].cache_state & DIRTY) 
    {
        cache->caches[*oldest_clean].cache_state = VALID;
    }
#endif
}

/**
 * @brief 根据块号查询缓存是否命中
 *
 * @param cache
 * @param blkno 请求的扇区号
 * @param index 如果命中，则返回 cache block 的索引号
 * @return HIT 或 NOT_HIT
 */
int cache_lookup(struct cache_dev *cache, ioreq_event *req, long *index) {
    int res = -1;
    long blkno = req->blkno;

    // 根据请求的块号找到对应的 set 号
    int set_number = hash_block(cache, blkno);

    long invalid, oldest_clean = -1;

    // 当前 set 的起始缓存块号
    long start_index = cache->assoc * set_number;

    /*
        查找是否命中。返回 index = -1 表示不命中
        返回的 index >= 0 的值表示命中，且 index 的值的含义为命中的缓存块号
    */
    find_valid_blkno(cache, blkno, set_number, index);

    if (*index >= 0) {
#ifdef DEBUG
        LOG(DEBUG, "set: %d, cache block: %ld, " LOG_INFO_BORDER_STYLE "Hit" LOG_STYLE_CLEAR "\n", set_number, *index);
#endif

        res = HIT;
    }

    /*
        不命中则查找空闲块，返回的 invalid 结果就是空闲块的缓存块号，如果为 -1，表示没有空闲块了
    */
    if (*index < 0) {
        invalid = find_invalid_blkno(cache, set_number);

        if (invalid == -1) {
            // set 中已经满了，这时候需要进行替换
            find_reclaim_blkno(cache, start_index, &oldest_clean);
        }

        /*
         * Cache miss :
         * We can't choose an entry marked INPROG, but choose the oldest
         * INVALID or the oldest VALID entry.
         */

        *index = start_index + cache->assoc;
        if (invalid != -1) {
            *index = invalid;
        } else if (oldest_clean != -1) {
            *index = oldest_clean;
        }

        // index 记录替换下来的缓存块号索引，返回为有效或者是无效
        if (*index < (start_index + cache->assoc)) {
            res = NOT_HIT;
        } else {
            res = CACHE_ERROR;
        }

#ifdef DEBUG
        if (oldest_clean == -1) {
            LOG(DEBUG, "set: %d, cache block: %ld, " LOG_ERROR_BORDER_STYLE "Not Hit" LOG_STYLE_CLEAR "\n", set_number, *index);
        } else {
            LOG(DEBUG, "set: %d, Evition cache block: %ld, " LOG_ERROR_BORDER_STYLE "Not Hit" LOG_STYLE_CLEAR "\n", set_number, oldest_clean);
        }
#endif
    }

    // 每次新来请求，根据不同缓存策略调整该请求所分配的缓存块的位置
    // if (req->flags & WRITE) {
    switch (cache->cache_policy) {
        case CACHE_FIFO:
            break;
        case CACHE_LRU:
            cache_lru_accessed(cache, *index);
            break;
        case CACHE_DOUBLE_LRU:
            cache_double_linklist_lru_accessed(cache, *index);
            break;
        default:
            break;
    }
        // }

#ifdef DEBUG
    struct cache_set *cache_set = &cache->cache_sets[hash_block(cache, blkno)];
    struct cache_block *cacheblk = &cache->caches[*index];
    LOG(DEBUG, "lru_next: %d, lru_prev: %d, hot_lru_tail: %d, hot_lru_head: %d, warm_lru_tail: %d, warm_lru_head: %d\n", cacheblk->lru_next, cacheblk->lru_prev, cache_set->hotlist_lru_tail, cache_set->hotlist_lru_head, cache_set->warmlist_lru_tail, cache_set->warmlist_lru_head);
#endif

    return res;
}

/**
 * @brief lru链表的初始化，每个 set 的 hot 链表在 blockno 编号大的一端，
 *        warm 链表在小的一端，invalid_head 便从 hot 开始分配
 *
 * @param cache
 */
static void cache_init_lru_lists(struct cache_dev *cache) {
    int start_index = 0;
    int i = 0;
    int j = 0;
    int block_index = 0;
    // hot块的个数

    int hot_blocks_set = (cache->lru_hot_pct * cache->assoc) / 100;

    struct cache_block *cacheblk;
    // 将所有的cache进行初始化
    for (i = 0; i < cache->set_numbers; i++) {
        // 每一个set的起始索引
        start_index = i * cache->assoc;
        // for (j = 0; j < hot_blocks_set; j++) {
        // 双链 LRU，加入到 Hot 链上
        // for (j = 0; j < cache->assoc - hot_blocks_set; j++) {
        // 单链 LRU，全部为 Hot 链
        for (j = 0; j < 0; j++) {
            block_index = start_index + j;
            cacheblk = &cache->caches[block_index];
            cacheblk->lru_prev = CACHE_LRU_NULL;
            cacheblk->lru_next = CACHE_LRU_NULL;
            // cacheblk->lru_state = LRU_HOT;
            cacheblk->lru_state = LRU_WARM;

            // 添加到Lru端
            cache_reclaim_add_block_to_list_lru(cache, block_index);
        }

        // for (; j < cache->assoc; j++) {
        for (; j < cache->assoc; j++) {
            block_index = start_index + j;
            cacheblk = &cache->caches[block_index];
            cacheblk->lru_prev = CACHE_LRU_NULL;
            cacheblk->lru_next = CACHE_LRU_NULL;
            // cacheblk->lru_state = LRU_WARM;
            cacheblk->lru_state = LRU_HOT;
            cache_reclaim_add_block_to_list_lru(cache, block_index);
        }
    }
}

/**
 * @brief 初始化缓存结构体(cache_dev)
 *
 * @param cache
 */
void cache_initialize(struct cache_dev *cache) {
    cache->tape_eod = 0;
    cache->stat.reads = 0;
    cache->stat.readblocks = 0;
    cache->stat.readhitsfull = 0;
    cache->stat.readmisses = 0;
    cache->stat.popwrites = 0;
    cache->stat.popwriteblocks = 0;
    cache->stat.writes = 0;
    cache->stat.writeblocks = 0;
    cache->stat.writehitsfull = 0;
    cache->stat.writemisses = 0;
    cache->stat.maxbufferspace = 0;
    cache->stat.uncachedRead = 0;
    cache->stat.uncacheWrite = 0;

    cache->lru_hot_blocks = 0;
    cache->lru_warm_blocks = 0;

    // hot 链表的比例
    cache->lru_hot_pct = CACHE_LRU_HOT_PCT_DEFAULT;

    // 将所有的 cache 进行初始化
    for (int i = 0; i < cache->set_numbers; i++) {
        cache->cache_sets[i].set_fifo_next = i * cache->assoc;
        cache->cache_sets[i].set_clean_next = i * cache->assoc;

        cache->cache_sets[i].fallow_tstamp = 0;
        cache->cache_sets[i].fallow_next_cleaning = 0;

        // clean 的头部就是从头开始
        cache->cache_sets[i].nr_dirty = 0;
        cache->cache_sets[i].clean_inprog = 0;

        // hot链表的头部
        cache->cache_sets[i].hotlist_lru_head = CACHE_LRU_NULL;
        cache->cache_sets[i].hotlist_lru_tail = CACHE_LRU_NULL;

        // LRU warm 链表的实现
        cache->cache_sets[i].warmlist_lru_head = CACHE_LRU_NULL;
        cache->cache_sets[i].warmlist_lru_tail = CACHE_LRU_NULL;

        // invalid 链表和 hash 桶
        cache->cache_sets[i].invalid_head = CACHE_LRU_NULL;
        cache->cache_sets[i].lru_hot_blocks = 0;
        cache->cache_sets[i].lru_warm_blocks = 0;

        // 根据 assoc 给 hash_bucket 分配空间
        cache->cache_sets[i].hash_buckets = (long *)malloc(cache->assoc * sizeof(long));
        for (int j = 0; j < cache->assoc; j++) {
            cache->cache_sets[i].hash_buckets[j] = CACHE_LRU_NULL;
        }
    }

    // 初始化 LRU 链表
    cache_init_lru_lists(cache);

    // 将所有 cache 中一个 set 排列起来，组成一个 lru free
    for (int i = 0; i < cache->cache_numbers; i++) {
        cache->caches[i].cache_state = INVALID;
        cache->caches[i].index = i;
        cache->caches[i].blkno = 0;
        // cache_reclaim_lru_movetail(cache, i);
        // 每一个插入到无效链表里面
        cache->caches[i].hash_next = CACHE_LRU_NULL;
        cache->caches[i].hash_prev = CACHE_LRU_NULL;
        cache_invalid_insert(cache, i);
    }

    /* Cleaning Thresholds */
    cache->dirty_thresh_set = (cache->assoc * DIRTY_THRESH_DEF) / 100;

    // dirty块占据百分之20的时候开始
    cache->max_clean_ios_total = MAX_CLEAN_IOS_TOTAL;
    cache->max_clean_ios_set = MAX_CLEAN_IOS_SET;
}

/**
 * @brief 给定读请求是否命中
 *
 * @param cache
 * @param req
 * @param index
 * @return 命中返回 1，不命中返回 0
 */
static int cache_isreadhit(struct cache_dev *cache, ioreq_event *req, long *index) {
    struct cache_block *cacheblk;
    int res;
    int flag = 0;

    // 初始化cache_hit为0
    req->cache_hit = 0;
    
    // 查找请求的块号是否在缓存中
    res = cache_lookup(cache, req, index);

    if (res != CACHE_ERROR) {
        if (res == HIT) {
            // 命中缓存
            cache->stat.readhitsfull++;
            flag = 1;
            req->cache_hit = 1;
        } else if (res == NOT_HIT) {
            // 未命中缓存
            cache->stat.readmisses++;
            flag = 0;
        }
    }

// 打印 hash 桶的信息
#ifdef DEBUG
    LOG(DEBUG, "hash:");
    int setnum = hash_block(cache, req->blkno);
    struct cache_set *cache_set = &(cache->cache_sets[setnum]);
    struct cacheblock *cacheblk2;
    for (int i = 0; i < cache->set_numbers; i++) {
        long cache_block = cache_set->hash_buckets[i];
        cacheblk2 = &(cache->caches[cache_block + cache->assoc * setnum]);
        if (cache_block == CACHE_LRU_NULL) {
            printf(" null");
        } else {
            printf(" %ld", cache_block);
            while (cacheblk2->hash_next != CACHE_LRU_NULL) {
                printf("->%ld", cacheblk2->hash_next);
                cacheblk2 = &(cache->caches[cacheblk2->hash_next]);
            }
        }
    }
    printf("\n");
#endif
    return flag;
}

/**
 * @brief 给定请求是否写命中
 *
 * @param cache
 * @param req
 * @param index 如果写命中，则返回 cache block 的索引
 * @return 命中返回 1，不命中返回 0
 */
static int cache_iswritehit(struct cache_dev *cache, ioreq_event *req, long *index) {
    struct cache_block *cacheblk;
    int res;
    int flag = 0;

    /*
        如果写命中 index 返回的就是命中的缓存块号，返回的 res 为 HIT
        如果不命中 index 返回的就是要替换的缓存块号，返回的 res 为 NOT_HIT
        函数执行失败，res 返回 -1
    */
    res = cache_lookup(cache, req, index);
    req->cache_hit = 0;  // lzq

    // res == -1 表示出错
    if (res != CACHE_ERROR) {
        cacheblk = &cache->caches[*index];

        // 写命中的场景
        if (res == HIT) {
            // 写直通模式，不设置脏标志
            cacheblk->blkno = req->blkno;
            cache->stat.writehitsfull++;
            flag = 1;
            req->cache_hit = 1;  // lzq
        } else if (res == NOT_HIT) {
            /*
                不命中场景，缓存块是 INVALID 或者是请求块号不同的 VALID 块
            */
            cache->stat.writemisses++;
            flag = 0;
        }
    }

// 打印 hash 桶的信息
#ifdef DEBUG
    LOG(DEBUG, "hash:");
    int setnum = hash_block(cache, req->blkno);
    struct cache_set *cache_set = &(cache->cache_sets[setnum]);
    struct cacheblock *cacheblk2;
    for (int i = 0; i < cache->assoc; i++) {
        long cache_block = cache_set->hash_buckets[i];
        // cacheblk2 = &(cache->caches[cache_block + cache->assoc * setnum]);
        cacheblk2 = &(cache->caches[cache_block]);
        if (cache_block == CACHE_LRU_NULL) {
            printf(" null");
        } else {
            printf(" %ld", cache_block);
            while (cacheblk2->hash_next != CACHE_LRU_NULL) {
                printf("->%ld", cacheblk2->hash_next);
                cacheblk2 = &(cache->caches[cacheblk2->hash_next]);
            }
        }
    }
    printf("\n");
#endif

    return flag;
}

/**
 * @brief 执行缓存流程
 *
 * 功能说明：
 *  1. 首先判断请求在缓存中是否命中，读请求调用 cache_isreadhit，写请求调用 cache_iswritehit。
 *  2. 若命中，返回命中的缓存块。
 *  3. 若未命中，则：
 *      - 如果是读请求，从磁带读取数据到缓存中，替换缓存块；
 *      - 如果是写请求，同时写入缓存和磁带，更新映射表。
 *
 * @param cache 指向cache设备结构体的指针
 * @param req   请求结构体指针，其中包含 blkno、flags 以及 cache_hit 字段
 * @param mapping 地址映射表指针，用于逻辑地址到物理地址的转换
 * @return      分配给请求的缓存块指针；若出错则返回NULL
 */
void *hmsys_cache_access(struct cache_dev *cache, ioreq_event *req, address_mapping_t *mapping) {
    int hit = 0;
    long index = -1;
    uint32_t phy_addr = 0;

    // 首先进行逻辑地址到物理地址的转换
    if (req->flags == READ) {
        // 读操作需要先检查映射表中是否存在映射
        uint64_t logical_addr = req->blkno * BLOCK_SIZE;
        phy_addr = map_log_to_phy(mapping, logical_addr, 0); // 0表示读操作
        
        if (phy_addr == (uint32_t)-1) {
            // 映射不存在，无法读取
            LOG(ERROR, "Read operation on unmapped address: %lu\n", 
                logical_addr);
            return NULL;
        }
        
        // 在缓存中查找是否命中
        hit = cache_isreadhit(cache, req, &index);
    } else if (req->flags == WRITE) {
        // 写操作总是创建新的映射
        uint64_t logical_addr = req->blkno * BLOCK_SIZE;
        phy_addr = map_log_to_phy(mapping, logical_addr, 1); // 1表示写操作
        
        // 在缓存中查找是否命中（写操作通常不会命中，因为每次写都是不覆盖的）
        hit = cache_iswritehit(cache, req, &index);
    } else {
        /* 未知的请求类型 */
        fatal("Unknown request type", "");
        return NULL;
    }

    /* 检查返回的索引是否合法 */
    if (index < 0 || index >= cache->cache_numbers) {
        fatal("Invalid cache block index", "");
        return NULL;
    }

    struct cache_block *blk = &cache->caches[index];

    if (req->flags == READ) {
        if (hit) {
            // 读命中，直接返回数据
            LOG(DEBUG, "Read hit: logical=%ld, physical=%u\n", 
                req->blkno, phy_addr);
        } else {
            // 读未命中
            LOG(DEBUG, "Read miss: logical=%ld, physical=%u\n", 
                req->blkno, phy_addr);
            
            // 从磁带读取数据
            LOG(DEBUG, "Read from tape: physical=%u\n", 
                phy_addr);
            
            // 判断是否采用懒更新策略
            int update_cache = 1; // 默认更新缓存
            
            if (cache->lazy_update) {
                // 懒更新策略：检查是否有足够的空闲空间
                int set_number = hash_block(cache, req->blkno);
                long invalid = find_invalid_blkno(cache, set_number);
                
                if (invalid == -1) {
                    // 没有空闲块，不更新缓存
                    LOG(INFO, "No free cache block, skip updating cache (lazy update)\n");
                    update_cache = 0;
                }
            }
            
            if (update_cache) {
                // 替换缓存块
                if (blk->cache_state & VALID) {
                    // 从哈希表中移除
                    cache_hash_remove(cache, index);
                }
                
                // 更新缓存块信息
                blk->blkno = req->blkno;
                blk->phy_addr = phy_addr;
                blk->cache_state = VALID; // 设置为有效
                
                // 将更新后的块挂接到哈希链中
                cache_hash_insert(cache, index);
                
                LOG(DEBUG, "Updated cache with data from tape: logical=%ld, physical=%u\n", 
                    req->blkno, phy_addr);
            }
        }
    } else if (req->flags == WRITE) {
        // 写操作处理
        if (hit) {
            // 写命中（不应该发生，因为每次写都是不覆盖的）
            LOG(ERROR, "Unexpected write hit: logical=%ld, physical=%u\n", 
                req->blkno, phy_addr);
        } else {
            // 写未命中
            LOG(DEBUG, "Write miss: logical=%ld, physical=%u\n", 
                req->blkno, phy_addr);
            
            // 替换缓存块
            if (blk->cache_state & VALID) {
                // 从哈希表中移除
                cache_hash_remove(cache, index);
            }
            
            // 更新缓存块信息
            blk->blkno = req->blkno;
            blk->phy_addr = phy_addr;
            blk->cache_state = VALID; // 设置为有效
            
            // 将更新后的块挂接到哈希链中
            cache_hash_insert(cache, index);
            
            // 同时写入磁带
            LOG(DEBUG, "Write to tape: logical=%ld, physical=%u\n", 
                req->blkno, phy_addr);
            // 这里应该调用写磁带函数
        }
    }
    
    return blk;
}