/**
 * @Author      : CherryYang
 * @Date        : 2025-01-24 16:30:00
 * @FilePath    : /hmsys/src/test_mapping.c
 * @Description : 测试地址映射层功能
 */

#include "cache_layer.h"
#include "driver.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// 测试映射表项的设置和获取
void test_mapping_entry() {
    printf("\n===== 测试映射表项操作 =====\n");
    
    // 测试设置和获取物理地址
    uint32_t physical_addr = 0x1234567; // 一个27位的物理地址
    uint32_t flags = MAP_VALID | MAP_DIRTY; // 有效且脏
    uint32_t entry = set_mapping_entry(physical_addr, flags);
    
    uint32_t got_addr = get_physical_addr(entry);
    uint32_t got_flags = get_flags(entry);
    
    printf("设置: 物理地址=0x%x, 标志位=0x%x\n", physical_addr, flags);
    printf("获取: 物理地址=0x%x, 标志位=0x%x\n", got_addr, got_flags);
    
    assert(got_addr == physical_addr);
    assert(got_flags == flags);
    printf("测试通过: 映射表项的设置和获取正确\n");
}

// 测试初始化和释放映射表
void test_init_free() {
    printf("\n===== 测试初始化和释放映射表 =====\n");
    
    address_mapping_t *mapping = init_address_mapping();
    assert(mapping != NULL);
    assert(mapping->table != NULL);
    assert(mapping->next_tape_addr == 0);
    assert(mapping->entry_count == 0);
    
    printf("映射表初始化成功\n");
    
    free_address_mapping(mapping);
    printf("映射表释放成功\n");
}

// 测试单个地址的映射
void test_single_mapping() {
    printf("\n===== 测试单个地址的映射 =====\n");
    
    address_mapping_t *mapping = init_address_mapping();
    
    // 测试写操作的映射
    uint64_t logical_addr = 0x100000;    // 逻辑地址，大小为1MB
    uint32_t physical_addr = map_log_to_phy(mapping, logical_addr, 1);
    
    printf("写操作: 逻辑地址=0x%lx -> 物理地址=0x%x\n", logical_addr, physical_addr);
    assert(physical_addr == 0); // 第一次映射，物理地址应该是0
    
    // 测试读操作的映射
    physical_addr = map_log_to_phy(mapping, logical_addr, 0); // 读操作
    printf("读操作: 逻辑地址=0x%lx -> 物理地址=0x%x\n", logical_addr, physical_addr);
    assert(physical_addr == 0); // 已经映射过，物理地址应该是0
    

    // 测试另一个写操作的映射
    uint64_t logical_addr2 = 0x140000; // 另一个逻辑地址
    uint32_t physical_addr2 = map_log_to_phy(mapping, logical_addr2, 1);
    
    printf("写操作: 逻辑地址=0x%lx -> 物理地址=0x%x\n", logical_addr2, physical_addr2);
    assert(physical_addr2 == 1); // 第二次映射，物理地址应该是1
    
    // 测试未映射地址的读操作
    uint64_t unmapped_addr = 0x180000; // 未映射的逻辑地址
    uint32_t physical_addr3 = map_log_to_phy(mapping, unmapped_addr, 0); // 读操作
    
    printf("读操作(未映射): 逻辑地址=0x%lx -> 物理地址=0x%x\n", unmapped_addr, physical_addr3);
    assert(physical_addr3 == (uint32_t)-1); // 未映射的读操作应该返回-1
    
    free_address_mapping(mapping);
}

// 测试IO请求的映射
void test_io_request_mapping() {
    printf("\n===== 测试IO请求的映射 =====\n");
    
    address_mapping_t *mapping = init_address_mapping();
    
    // 创建一个写请求
    ioreq_event write_req;
    memset(&write_req, 0, sizeof(ioreq_event));
    write_req.flags = WRITE;
    write_req.blkno = 0;     // 起始块号
    write_req.bcount = 2048;    // 2048个块，大小为1MB
    
    // 处理写请求
    int result = create_mapping(mapping, &write_req);
    assert(result == 0);
    printf("写请求映射成功\n");
    
    // 创建一个读请求，读取已映射的区域
    ioreq_event read_req;
    memset(&read_req, 0, sizeof(ioreq_event));
    read_req.flags = READ;
    read_req.blkno = 1024;      // 起始块号，在已映射区域内
    read_req.bcount = 512;      // 512个块，大小为256KB
    
    // 处理读请求
    result = create_mapping(mapping, &read_req);
    assert(result == 0);
    printf("读请求映射成功\n");
    
    // 创建一个读请求，读取未映射的区域
    ioreq_event read_req2;
    memset(&read_req2, 0, sizeof(ioreq_event));
    read_req2.flags = READ;
    read_req2.blkno = 2048;     // 起始扇区号，未映射区域
    read_req2.bcount = 512;     // 512个块，大小为256KB
    
    // 处理读请求
    result = create_mapping(mapping, &read_req2);
    assert(result == -1);
    printf("读取未映射区域失败，符合预期\n");
    
    // 打印映射表状态
    print_mapping_status(mapping);
    
    free_address_mapping(mapping);
}

// 测试跨越多个最小存储单位的请求
void test_cross_unit_request() {
    printf("\n===== 测试跨越多个最小存储单位的请求 =====\n");
    
    address_mapping_t *mapping = init_address_mapping();
    
    // 计算一个最小存储单位包含多少个扇区
    uint32_t sectors_per_unit = MIN_UNIT_SIZE / BLOCK_SIZE;
    printf("每个最小存储单位包含 %u 个扇区\n", sectors_per_unit);
    
    // 创建一个跨越多个最小存储单位的写请求
    ioreq_event write_req;
    memset(&write_req, 0, sizeof(ioreq_event));
    write_req.flags = WRITE;
    write_req.blkno = sectors_per_unit - 2; // 第一个单位的倒数第二个扇区
    write_req.bcount = 5;  // 5个扇区，跨越两个单位
    
    // 处理写请求
    int result = create_mapping(mapping, &write_req);
    assert(result == 0);
    printf("跨越单位的写请求映射成功\n");
    
    // 打印映射表状态
    print_mapping_status(mapping);
    
    free_address_mapping(mapping);
}

// 测试大量随机请求
void test_random_requests() {
    printf("\n===== 测试大量随机请求 =====\n");
    
    address_mapping_t *mapping = init_address_mapping();
    
    // 生成100个随机请求
    const int num_requests = 100;
    for (int i = 0; i < num_requests; i++) {
        ioreq_event req;
        memset(&req, 0, sizeof(ioreq_event));
        
        // 随机生成读写类型
        req.flags = (rand() % 2 == 0) ? READ : WRITE;
        
        // 随机生成起始扇区号和扇区数量
        req.blkno = rand() % 1000000;
        req.bcount = 1 + rand() % 20; // 1-20个扇区
        
        // 处理请求
        int result = create_mapping(mapping, &req);
        
        // 读请求可能失败，写请求应该总是成功
        if (req.flags == WRITE) {
            assert(result == 0);
        }
        
        if (i % 10 == 0) {
            printf("处理了 %d 个请求\n", i);
        }
    }
    
    // 打印映射表状态
    print_mapping_status(mapping);
    
    free_address_mapping(mapping);
}

int main() {
    // 设置随机数种子
    srand(time(NULL));
    
    printf("开始测试地址映射层...\n");
    
    // 运行各个测试
    test_mapping_entry();
    test_init_free();
    test_single_mapping();
    test_io_request_mapping();
    // test_cross_unit_request();
    // test_random_requests();
    
    printf("\n所有测试通过！\n");
    return 0;
} 