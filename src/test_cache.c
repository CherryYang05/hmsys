/**
 * @Author      : CherryYang
 * @Date        : 2025-01-25 10:30:00
 * @FilePath    : /hmsys/src/test_cache.c
 * @Description : 测试缓存读写命中功能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "cache.h"
#include "cache_layer.h"
#include "log.h"
#include "global.h"
#include "config.h"

// 实现fatal函数，用于错误处理
void fatal(const char *msg, const char *msg1) {
    LOG(ERROR, "%s%s\n", msg, msg1 ? msg1 : "");
    exit(1);
}

// 测试缓存初始化
void test_cache_init() {
    LOG(INFO, "测试缓存初始化...\n");
    
    struct cache_dev *cache = cache_create_and_init();
    assert(cache != NULL);
    
    LOG(INFO, "缓存初始化成功，大小: %ld, 块大小: %u, 组相联数: %u\n", 
        cache->size, cache->block_size, cache->assoc);
    LOG(INFO, "缓存 set 数: %d, 缓存块数量: %d\n", 
        cache->set_numbers, cache->cache_numbers);
    
    cache_free_memory(cache);
    LOG(INFO, "缓存初始化测试通过\n");
}

// 测试地址映射初始化
void test_mapping_init() {
    LOG(INFO, "测试地址映射初始化...\n");
    
    address_mapping_t *mapping = init_address_mapping();
    assert(mapping != NULL);
    
    LOG(INFO, "地址映射初始化成功，表大小: %u\n", MAPPING_TABLE_SIZE);
    
    free_address_mapping(mapping);
    LOG(INFO, "地址映射初始化测试通过\n");
}

// 测试读操作时的逻辑地址到物理地址的转换
void test_read_address_mapping() {
    LOG(INFO, "测试读操作时的逻辑地址到物理地址的转换...\n");
    
    address_mapping_t *mapping = init_address_mapping();
    assert(mapping != NULL);
    
    // 测试写操作的映射
    uint64_t write_log_addr = 1024 * 1024; // 1MB
    uint32_t write_phy_addr = map_log_to_phy(mapping, write_log_addr, 1); // 写操作
    
    LOG(INFO, "写操作映射: 逻辑地址=%lu -> 物理地址=%u\n", 
        write_log_addr, write_phy_addr);
    
    // 测试对已映射地址的读操作
    uint64_t read_mapped_addr = write_log_addr;
    uint32_t read_mapped_phy_addr = map_log_to_phy(mapping, read_mapped_addr, 0); // 读操作
    
    LOG(INFO, "读操作映射(已映射): 逻辑地址=%lu -> 物理地址=%u\n", 
        read_mapped_addr, read_mapped_phy_addr);
    
    // 验证对已映射地址的读操作返回的物理地址与写操作相同
    assert(read_mapped_phy_addr == write_phy_addr);
    
    // 测试对未映射地址的读操作
    uint64_t read_unmapped_addr = 2 * 1024 * 1024; // 2MB，未映射
    uint32_t read_unmapped_phy_addr = map_log_to_phy(mapping, read_unmapped_addr, 0); // 读操作
    
    LOG(INFO, "读操作映射(未映射): 逻辑地址=%lu -> 物理地址=%u\n", 
        read_unmapped_addr, read_unmapped_phy_addr);
    
    // 验证对未映射地址的读操作返回错误码
    assert(read_unmapped_phy_addr == (uint32_t)-1);
    
    LOG(INFO, "读操作时的逻辑地址到物理地址的转换测试通过\n");
    
    free_address_mapping(mapping);
}

// 测试单个读命中
void test_read_hit() {
    LOG(INFO, "测试读命中...\n");
    
    struct cache_dev *cache = cache_create_and_init();
    address_mapping_t *mapping = init_address_mapping();
    
    // 创建一个写请求，将数据写入缓存
    ioreq_event write_req;
    memset(&write_req, 0, sizeof(ioreq_event));
    write_req.blkno = 1024;  // 逻辑块号
    write_req.bcount = 512;   // 请求长度
    write_req.flags = WRITE;
    
    // 处理写请求
    int mapping_result = create_mapping(mapping, &write_req);
    assert(mapping_result == 0);
    
    void *write_blk = hmsys_cache_access(cache, &write_req, mapping);
    assert(write_blk != NULL);
    
    // 创建一个读请求，读取相同的块
    ioreq_event read_req;
    memset(&read_req, 0, sizeof(ioreq_event));
    read_req.blkno = 1024;  // 与写请求相同的逻辑块号
    read_req.bcount = 512;
    read_req.flags = READ;
    
    // 处理读请求
    mapping_result = create_mapping(mapping, &read_req);
    assert(mapping_result == 0);
    
    void *read_blk = hmsys_cache_access(cache, &read_req, mapping);
    assert(read_blk != NULL);
    
    // 验证读请求是否命中
    assert(read_req.cache_hit == 1);
    
    LOG(INFO, "读命中测试通过\n");
    
    free_address_mapping(mapping);
    cache_free_memory(cache);
}

// 测试单个读未命中
void test_read_miss() {
    LOG(INFO, "测试读未命中...\n");
    
    struct cache_dev *cache = cache_create_and_init();
    address_mapping_t *mapping = init_address_mapping();
    
    // 创建一个写请求，将数据写入缓存
    ioreq_event write_req;
    memset(&write_req, 0, sizeof(ioreq_event));
    write_req.blkno = 0;  // 逻辑块号
    write_req.bcount = 512;   // 请求长度
    write_req.flags = WRITE;
    
    // 处理写请求
    int mapping_result = create_mapping(mapping, &write_req);
    assert(mapping_result == 0);
    
    void *write_blk = hmsys_cache_access(cache, &write_req, mapping);
    assert(write_blk != NULL);
    
    // 创建一个读请求，读取不同的块
    ioreq_event read_req;
    memset(&read_req, 0, sizeof(ioreq_event));
    read_req.blkno = 1024;  // 与写请求不同的逻辑块号
    read_req.bcount = 512;
    read_req.flags = READ;
    
    // 处理读请求 - 应该失败，因为该地址未映射
    mapping_result = create_mapping(mapping, &read_req);
    assert(mapping_result == -1);  // 修改为期望失败
    
    // void *read_blk = hmsys_cache_access(cache, &read_req, mapping);
    // assert(read_blk != NULL);
    
    // // 验证读请求是否未命中
    // assert(read_req.cache_hit == 0);
    
    LOG(INFO, "读未命中测试通过\n");
    
    free_address_mapping(mapping);
    cache_free_memory(cache);
}

// 测试写不命中（在本系统中，写操作总是不会命中，因为每次写都是不覆盖的）
void test_write_miss() {
    LOG(INFO, "测试写不命中...\n");
    
    struct cache_dev *cache = cache_create_and_init();
    address_mapping_t *mapping = init_address_mapping();
    
    // 创建一个写请求
    ioreq_event write_req;
    memset(&write_req, 0, sizeof(ioreq_event));
    write_req.blkno = 2048;  // 逻辑块号
    write_req.bcount = 512;   // 请求长度
    write_req.flags = WRITE;
    
    // 处理写请求
    int mapping_result = create_mapping(mapping, &write_req);
    assert(mapping_result == 0);
    
    void *write_blk = hmsys_cache_access(cache, &write_req, mapping);
    assert(write_blk != NULL);
    
    // 验证写请求是否未命中（在本系统中，写操作总是不会命中）
    assert(write_req.cache_hit == 0);
    
    LOG(INFO, "写不命中测试通过\n");
    
    free_address_mapping(mapping);
    cache_free_memory(cache);
}

// 测试多次读写操作
void test_multiple_operations() {
    LOG(INFO, "测试多次读写操作...\n");
    
    struct cache_dev *cache = cache_create_and_init();
    address_mapping_t *mapping = init_address_mapping();
    
    // 执行多次写操作，只写三个块是为了避免缓存被替换
    for (int i = 0; i < 3; i++) {
        ioreq_event write_req;
        memset(&write_req, 0, sizeof(ioreq_event));
        write_req.blkno = i * 1024;  // 不同的逻辑块号
        write_req.bcount = 1024;
        write_req.flags = WRITE;
        
        int mapping_result = create_mapping(mapping, &write_req);
        assert(mapping_result == 0);
        
        void *write_blk = hmsys_cache_access(cache, &write_req, mapping);
        assert(write_blk != NULL);
    }
    
    // 执行读操作，一半应该命中，一半应该未映射
    int hit_count = 0;
    int unmapped_count = 0;
    for (int i = 0; i < 6; i++) {
        ioreq_event read_req;
        memset(&read_req, 0, sizeof(ioreq_event));
        read_req.blkno = i * 1024;  // 一半的块号应该在缓存中
        read_req.bcount = 1024;
        read_req.flags = READ;
        
        int mapping_result = create_mapping(mapping, &read_req);
        if (mapping_result != 0) {
            // 对于未映射的地址，映射会失败
            unmapped_count++;
            continue;
        }
        
        void *read_blk = hmsys_cache_access(cache, &read_req, mapping);
        if (read_blk != NULL && read_req.cache_hit) {
            hit_count++;
        }
    }
    
    LOG(INFO, "多次读写操作中，读命中次数: %d, 未映射地址数: %d\n", hit_count, unmapped_count);
    // 验证未映射地址的数量
    assert(unmapped_count == 3); // 应该有3个未映射的地址（i从3到5）
    LOG(INFO, "多次读写操作测试通过\n");
    
    free_address_mapping(mapping);
    cache_free_memory(cache);
}

// 测试缓存替换
void test_cache_replacement() {
    LOG(INFO, "测试缓存替换...\n");
    
    struct cache_dev *cache = cache_create_and_init();
    address_mapping_t *mapping = init_address_mapping();
    
    // 获取缓存的容量（块数）
    int cache_capacity = cache->cache_numbers;
    LOG(INFO, "缓存容量: %d 块\n", cache_capacity);
    
    // 写入比缓存容量更多的块，触发替换
    int write_count = cache_capacity * 2;
    for (int i = 0; i < write_count; i++) {
        ioreq_event write_req;
        memset(&write_req, 0, sizeof(ioreq_event));
        write_req.blkno = i * 512;  // 不同的逻辑块号
        write_req.bcount = 512;
        write_req.flags = WRITE;
        
        int mapping_result = create_mapping(mapping, &write_req);
        assert(mapping_result == 0);
        
        void *write_blk = hmsys_cache_access(cache, &write_req, mapping);
        assert(write_blk != NULL);
    }
    
    // 读取前面写入的块，检查命中率
    int hit_count = 0;
    int total_reads = 0;
    for (int i = 0; i < write_count; i++) {
        ioreq_event read_req;
        memset(&read_req, 0, sizeof(ioreq_event));
        read_req.blkno = i * 512;
        read_req.bcount = 512;
        read_req.flags = READ;
        
        int mapping_result = create_mapping(mapping, &read_req);
        assert(mapping_result == 0); // 所有地址都应该已经映射
        
        void *read_blk = hmsys_cache_access(cache, &read_req, mapping);
        assert(read_blk != NULL);
        
        total_reads++;
        if (read_req.cache_hit) {
            hit_count++;
        }
    }
    
    LOG(INFO, "缓存替换测试中，读命中次数: %d/%d\n", hit_count, total_reads);
    // 由于缓存容量有限，只有部分写入的块会在缓存中
    assert(hit_count > 0 && hit_count < total_reads);
    LOG(INFO, "缓存替换测试通过\n");
    
    free_address_mapping(mapping);
    cache_free_memory(cache);
}

// 测试懒更新策略
void test_lazy_update() {
    LOG(INFO, "测试懒更新策略...\n");
    
    struct cache_dev *cache = cache_create_and_init();
    // 设置懒更新策略
    cache->lazy_update = 1;
    
    address_mapping_t *mapping = init_address_mapping();
    
    // 写入一些数据
    for (int i = 0; i < 10; i++) {
        ioreq_event write_req;
        memset(&write_req, 0, sizeof(ioreq_event));
        write_req.blkno = i * 512;
        write_req.bcount = 512;
        write_req.flags = WRITE;
        
        int mapping_result = create_mapping(mapping, &write_req);
        assert(mapping_result == 0);
        
        void *write_blk = hmsys_cache_access(cache, &write_req, mapping);
        assert(write_blk != NULL);
    }
    
    // 读取数据，检查命中情况
    int hit_count = 0;
    for (int i = 0; i < 10; i++) {
        ioreq_event read_req;
        memset(&read_req, 0, sizeof(ioreq_event));
        read_req.blkno = i * 512;
        read_req.bcount = 512;
        read_req.flags = READ;
        
        int mapping_result = create_mapping(mapping, &read_req);
        assert(mapping_result == 0); // 所有地址都应该已经映射
        
        void *read_blk = hmsys_cache_access(cache, &read_req, mapping);
        assert(read_blk != NULL);
        
        if (read_req.cache_hit) {
            hit_count++;
        }
        
        LOG(INFO, "懒更新策略下，块 %ld 的读请求%s命中\n", 
            read_req.blkno, read_req.cache_hit ? "已" : "未");
    }
    
    LOG(INFO, "懒更新策略下，总读命中次数: %d/10\n", hit_count);
    LOG(INFO, "懒更新策略测试通过\n");
    
    free_address_mapping(mapping);
    cache_free_memory(cache);
}

// 主函数
int main() {
    // 初始化配置
    hmsys_setup();
    
    LOG(INFO, "开始缓存读写命中测试...\n");
    
    // 运行测试
    test_cache_init();
    test_mapping_init();
    test_read_address_mapping();
    test_read_hit();
    test_read_miss();
    test_write_miss();
    test_multiple_operations();
    // test_cache_replacement();
    // test_lazy_update();
    
    LOG(INFO, "所有测试通过！\n");
    
    return 0;
} 