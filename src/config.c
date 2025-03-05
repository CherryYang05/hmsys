/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 17:57:27
 * @FilePath    : /hmsys/src/config.c
 * @Description : 读取 toml 配置文件
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "toml.h"
#include "stats.h"
#include "log.h"
#include "trace.h"
#include "cache.h"

FILE *statsfile = NULL;
FILE *response = NULL;
FILE *tracefile = NULL;

// 全局缓存配置参数
struct cache_config {
    long size;                  // 缓存总大小(以系统最小粒度为单位, 512B)
    unsigned int block_size;    // 每个缓存块有多少个512B
    unsigned int assoc;         // 组相连组数
    int cache_policy;           // 缓存替换算法，FIFO=0，LRU=1
    int write_scheme;           // 写策略
    int lru_hot_pct;            // hot链表的比例
    int dirty_thresh;           // 脏块阈值
    int max_clean_ios_total;    // 总清理IO数量
    int max_clean_ios_set;      // 每个set的清理IO数量
    int lazy_update;            // 是否采用懒更新策略
} cache_config;

/**
 * @brief 读取并解析 toml 配置文件
 * 
 * @param config_file 
 */
static void hmsys_read_config(const char *config_file) {
    FILE *fp;
    char errbuf[200];

    // 1. 读取配置文件
    fp = fopen(config_file, "r");
    if (!fp) {
        fatal("cannot open config.toml - ", strerror(errno));
    }

    toml_table_t *conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!conf) {
        fatal("cannot parse - ", errbuf);
    }

    // 2. 转化表单
    toml_table_t *cache = toml_table_in(conf, "cache");
    if (!cache) {
        fatal("missing [cache] section", "");
    }

    // 3. 提取缓存参数
    // 缓存大小
    toml_datum_t size = toml_int_in(cache, "size");
    if (!size.ok) {
        LOG(WARNING, "cannot read cache.size, using default value 1048576\n");
        cache_config.size = 1048576; // 默认1GB (以512B为单位)
    } else {
        cache_config.size = size.u.i;
    }

    // 缓存块大小
    toml_datum_t block_size = toml_int_in(cache, "block_size");
    if (!block_size.ok) {
        LOG(WARNING, "cannot read cache.block_size, using default value 8\n");
        cache_config.block_size = 8; // 默认4KB (8个512B)
    } else {
        cache_config.block_size = block_size.u.i;
    }

    // 组相连路数
    toml_datum_t assoc = toml_int_in(cache, "assoc");
    if (!assoc.ok) {
        LOG(WARNING, "cannot read cache.assoc, using default value 16\n");
        cache_config.assoc = 16; // 默认16路组相连
    } else {
        cache_config.assoc = assoc.u.i;
    }

    // 缓存替换策略
    toml_datum_t policy = toml_int_in(cache, "cache_policy");
    if (!policy.ok) {
        LOG(WARNING, "cannot read cache.cache_policy, using default value 1 (LRU)\n");
        cache_config.cache_policy = CACHE_LRU; // 默认LRU
    } else {
        cache_config.cache_policy = policy.u.i;
    }

    // 写策略
    toml_datum_t write_scheme = toml_int_in(cache, "write_scheme");
    if (!write_scheme.ok) {
        LOG(WARNING, "cannot read cache.write_scheme, using default value 3 (WRITE_BACK)\n");
        cache_config.write_scheme = CACHE_WRITE_BACK; // 默认写回
    } else {
        cache_config.write_scheme = write_scheme.u.i;
    }

    // hot链表比例
    toml_datum_t lru_hot_pct = toml_int_in(cache, "lru_hot_pct");
    if (!lru_hot_pct.ok) {
        LOG(WARNING, "cannot read cache.lru_hot_pct, using default value 75\n");
        cache_config.lru_hot_pct = CACHE_LRU_HOT_PCT_DEFAULT; // 默认75%
    } else {
        cache_config.lru_hot_pct = lru_hot_pct.u.i;
    }

    // 脏块阈值
    toml_datum_t dirty_thresh = toml_int_in(cache, "dirty_thresh");
    if (!dirty_thresh.ok) {
        LOG(WARNING, "cannot read cache.dirty_thresh, using default value 90\n");
        cache_config.dirty_thresh = DIRTY_THRESH_DEF; // 默认90%
    } else {
        cache_config.dirty_thresh = dirty_thresh.u.i;
    }

    // 总清理IO数量
    toml_datum_t max_clean_ios_total = toml_int_in(cache, "max_clean_ios_total");
    if (!max_clean_ios_total.ok) {
        LOG(WARNING, "cannot read cache.max_clean_ios_total, using default value 4\n");
        cache_config.max_clean_ios_total = MAX_CLEAN_IOS_TOTAL; // 默认4
    } else {
        cache_config.max_clean_ios_total = max_clean_ios_total.u.i;
    }

    // 每个set的清理IO数量
    toml_datum_t max_clean_ios_set = toml_int_in(cache, "max_clean_ios_set");
    if (!max_clean_ios_set.ok) {
        LOG(WARNING, "cannot read cache.max_clean_ios_set, using default value 2\n");
        cache_config.max_clean_ios_set = MAX_CLEAN_IOS_SET; // 默认2
    } else {
        cache_config.max_clean_ios_set = max_clean_ios_set.u.i;
    }

    // 是否采用懒更新策略
    toml_datum_t lazy_update = toml_int_in(cache, "lazy_update");
    if (!lazy_update.ok) {
        LOG(WARNING, "cannot read cache.lazy_update, using default value 0\n");
        cache_config.lazy_update = 0; // 默认不采用懒更新
    } else {
        cache_config.lazy_update = lazy_update.u.i;
    }

    LOG(INFO, "Cache config: size=%ld, block_size=%u, assoc=%u, policy=%d, write_scheme=%d\n", 
        cache_config.size, cache_config.block_size, cache_config.assoc, 
        cache_config.cache_policy, cache_config.write_scheme);

    // 4. 释放内存
    toml_free(conf);
}

/**
 * @brief 获取缓存配置参数
 * 
 * @param cache 缓存设备指针
 */
void get_cache_config(struct cache_dev *cache) {
    cache->size = cache_config.size;
    cache->block_size = cache_config.block_size;
    cache->assoc = cache_config.assoc;
    cache->cache_policy = cache_config.cache_policy;
    cache->writescheme = cache_config.write_scheme;
    cache->lru_hot_pct = cache_config.lru_hot_pct;
    cache->dirty_thresh_set = (cache->assoc * cache_config.dirty_thresh) / 100;
    cache->max_clean_ios_total = cache_config.max_clean_ios_total;
    cache->max_clean_ios_set = cache_config.max_clean_ios_set;
    cache->lazy_update = cache_config.lazy_update;
}

void hmsys_setup() {
    hmsys_read_config(CONFIG_PATH);

#ifdef TRACE_PATH
    // 只有在定义了TRACE_PATH时才打开trace文件
    tracefile = fopen(TRACE_PATH, "r");
    if (!tracefile) {
        fatal("Can not open trace file - ", strerror(errno));
    }
#endif

#ifdef STATS_PATH
    // 只有在定义了STATS_PATH时才打开stats文件
    statsfile = fopen(STATS_PATH, "w+");
    if (!statsfile) {
        fatal("Can not open stats file - ", strerror(errno));
    }
#endif

#ifdef RESPONSE_PATH
    // 只有在定义了RESPONSE_PATH时才打开response文件
    response = fopen(RESPONSE_PATH, "w+");
    if (!response) {
        fatal("Can not open response file - ", strerror(errno));
    }
#endif

    LOG(INFO, "hmsys init done.\n");
}
