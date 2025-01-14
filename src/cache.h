/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:15:30
 * @FilePath    : /hmsys/src/cache.h
 * @Description : 缓存层逻辑的头文件
 */

#ifndef HMSYS_CACHE_H
#define HMSYS_CACHE_H

#define CACHE_FIFO          0
#define CACHE_LRU           1
#define CACHE_DOUBLE_LRU    2

#define CACHE_LRU_NULL      -1


// cache 块的状态
#define INVALID     (1 << 0)    /* 未使用或无效的块，默认为初始状态 */
#define VALID       (1 << 1)    /* 已使用的有效块 */
#define DIRTY       (1 << 2)    /* 脏块，需要写回 */

// 设置标志位
#define SET_INVALID(flags)      ((flags) |= INVALID)
#define SET_VALID(flags)        ((flags) |= VALID)
#define SET_DIRTY(flags)        ((flags) |= DIRTY)

// 默认初始化标志位，即清除所有标志位
#define DEFAULT_FLAGS(flags)    ((flags) = 0)

// 清除特定标志位
#define CLEAN_INVALID(flags)    ((flags) &= ~INVALID)
#define CLEAN_VALID(flags)      ((flags) &= ~VALID)
#define CLEAN_DIRTY(flags)      ((flags) &= ~DIRTY)

// 命中状态
#define HIT         0x0001
#define NOT_HIT     0x0002

#define CACHE_ERROR -1

// Lru的状态
#define LRU_HOT     0x0001  /* On Hot LRU List */
#define LRU_WARM    0x0002 /* On Warm LRU List */


#define CACHE_LRU_HOT_PCT_DEFAULT   75
#define DIRTY_THRESH_MIN            10
#define DIRTY_THRESH_MAX            90
#define DIRTY_THRESH_DEF            90

#define MAX_CLEAN_IOS_TOTAL         4
#define MAX_CLEAN_IOS_SET           2


/**
 * @brief cache 块有两个状态 cache的状态，以及LRU的状态。
 * 
 */
struct cache_block {
    int cache_state;            /* cache 块的状态，包括 VALID（有效），
                                   INVALID（空闲），DIRTY（脏） */
    int lru_prev;               /* LRU 链表前驱指针 */
    int lru_next;               /* LRU 链表后继指针 */
    long blkno;                 /* 请求的块号 */
    int blkcnt;
    int lru_state;              /* LRU 链表状态，为 HOT 或 WARM
                                 （只有双链 LRU 策略该字段才有效） */
    int index;                  /* 缓存块号（全局），对应的SSD的索引下标 */
    long hash_prev, hash_next;  /* 当该缓存块为 INVALID，表示空闲缓存块链表的前置指针；
                                   当该缓存块为非 INVALID，表示哈希表中同一个 value 
                                   所链成的链表的前置指针 */
};

/**
 * @brief cache_set 的结构
 * 
 */
struct cache_set {
    int set_fifo_next;
    int set_clean_next;  // 清理set的索引
    int clean_inprog;    // 正在清理
    int nr_dirty;        // 脏块的数量
    int dirty_fallow;

    unsigned long fallow_tstamp;
    unsigned long fallow_next_cleaning;

    int hotlist_lru_head, hotlist_lru_tail;     /* 每个 set 中 LRU 链表的头尾指针（名称中
                                                   包含 hot 的意义是为了双链 LRU 的拓展，
                                                   双链 LRU 还有一个 warm 链表，下同） */
    int warmlist_lru_head, warmlist_lru_tail;  // LRU 的 warm 链表
    int lru_hot_blocks, lru_warm_blocks;

    long *hash_buckets;                         /* 哈希桶，用来查找缓存是否命中 */
    long invalid_head;                          /* 空闲缓存块链表头指针 */
};


/**
 * @brief cache 设备的统计信息
 * 
 */
struct cache_dev_stats {
    int reads;              /* 读请求 */
    int readblocks;         /* 读请求的块数 */
    int readhitsfull;       /* 读请求命中 */
    int readmisses;         /* 读请求未命中 */
    int popwrites;          /* 从 cache 中弹出的写请求 */ 
    int popwriteblocks;     /* 从 cache 中弹出的写请求的块数 */
    int writes;             /* 写请求 */
    int writeblocks;        /* 写请求的块数 */
    int writehitsfull;      /* 写请求命中 */
    int writemisses;        /* 写请求未命中 */
    int maxbufferspace;     /* 最大缓存空间 */
    int uncachedRead;       /* 未缓存读 */
    int uncacheWrite;       /* 未缓存写 */
};


/**
 * @brief cache 设备的结构，包括 cache 设备的统计信息、cache 设备的大小、
 * 
 */
struct cache_dev {
    struct cache_dev_stats stat;
    // struct ioq *(**queuefind)(void *, int);    /* to get ioqueue ptr for dev*/
    long size;                      /* 缓存总大小(以系统最小粒度为单位, 512B) */
    int cache_devno;                /* device used for cache */
    int vol_devno;                  /* device for which cache is used */
    int maxreqsize; 
    int writescheme;                /* 写策略 */
    int flush_policy;   
    unsigned int assoc;             /* 组相连组数 */
    double flush_period;    
    int set_numbers;                /* set 的数量 */
    int cache_numbers;              /* cache 块的数量 */
    struct cache_block *caches;      /* cache 数组，存放所有的 cache 块 */
    struct cache_set *cache_sets;   /* cache set 数组，存放所有的 cache set */
    int dirty_thresh_set;           /* Per set dirty threshold to start cleaning */
    int cache_policy;               /* 缓存替换算法，FIFO=0，LRU=1 */
    unsigned int block_size;        /* 每个缓存块有多少个 512B */
    int lru_hot_pct;
    int lru_warm_blocks;
    int lru_hot_blocks;
    long tape_eod;
    int prefetch_enable;            /* 是否预取 */
    int prefetch_thresh;            /* 预取阈值 */
    int max_clean_ios_set;          /* Max cleaning IOs per set */
    int max_clean_ios_total;        /* Total max cleaning IOs */
};


// static void cache_initialize(struct cache_dev *cache);
// void cache_init_lru_lists(struct cache_dev *cache);
// static int hash_block(struct cache_dev *cache, long blkno);
// static void find_valid_blkno(struct cache_dev *cache, long blkno, int set_number, int *index);
// static int find_invalid_blkno(struct cache_dev *cache, int set);
// static int cache_lookup(struct cache_dev *cache, long blkno, int *index);


#endif