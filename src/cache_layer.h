/**
 * @Author      : CherryYang
 * @Date        : 2025-01-24 15:58:00
 * @FilePath    : /hmsys/src/cache_layer.h
 * @Description : 缓存模块头文件，包含地址映射层的定义
 */

#ifndef HMSYS_CACHE_LAYER_H
#define HMSYS_CACHE_LAYER_H

#include "driver.h"
#include "global.h"

// 缓存映射表相关参数定义
#define TAPE_SIZE_TB 18                      // 磁带总大小，单位TB
#define MIN_UNIT_SIZE (256 * 1024)           // 最小存储单位，256KB
#define MIN_UNIT_SHIFT 18                    // 最小存储单位对应的位移量 (2^18 = 256KB)
#define LOGICAL_ADDR_BITS 45                 // 逻辑地址位数 (18TB 需要 45 位)
#define MAPPING_INDEX_BITS 27                // 映射表索引位数 (45-18=27)
#define MAPPING_TABLE_SIZE (1 << MAPPING_INDEX_BITS) // 映射表大小

// 映射表项标志位
#define MAP_VALID 1 << 0                        // 有效位 (第0位)
#define MAP_DIRTY 1 << 1                        // 修改位 (第1位)

// 映射表项结构 (32位)
typedef struct {
    uint32_t entry;                          // 高27位为物理地址，低2位为标志位
} mapping_entry_t;

// 地址映射表结构
typedef struct {
    mapping_entry_t *table;                  // 映射表数组
    uint32_t next_tape_addr;                 // 下一个可用的磁带物理地址
    uint32_t entry_count;                    // 已使用的映射表项数量
} address_mapping_t;

/**
 * @brief 初始化地址映射表
 * 
 * @return address_mapping_t* 初始化的映射表指针，失败返回NULL
 */
address_mapping_t* init_address_mapping();

/**
 * @brief 释放地址映射表
 * 
 * @param mapping 映射表指针
 */
void free_address_mapping(address_mapping_t *mapping);

/**
 * @brief 将逻辑地址映射到磁带物理地址
 * 
 * @param mapping 映射表指针
 * @param logical_addr 逻辑地址
 * @param is_write 是否为写操作
 * @return uint32_t 映射后的物理地址
 */
uint32_t map_log_to_phy(address_mapping_t *mapping, uint64_t logical_addr, int is_write);

/**
 * @brief 处理IO请求，创建映射关系
 * 
 * @param mapping 映射表指针
 * @param req IO请求
 * @return int 成功返回0，失败返回-1
 */
int create_mapping(address_mapping_t *mapping, ioreq_event *req);

/**
 * @brief 打印映射表状态
 * 
 * @param mapping 映射表指针
 */
void print_mapping_status(address_mapping_t *mapping);

/**
 * @brief 获取映射表项的物理地址部分
 * 
 * @param entry 映射表项
 * @return uint32_t 物理地址
 */
uint32_t get_physical_addr(uint32_t entry);

/**
 * @brief 获取映射表项的标志位
 * 
 * @param entry 映射表项
 * @return uint32_t 标志位
 */
uint32_t get_flags(uint32_t entry);

/**
 * @brief 从逻辑地址中提取映射表索引
 * 
 * @param log_addr 逻辑地址
 * @return uint32_t 映射表索引
 */
uint32_t get_mapping_index(uint64_t log_addr);

/**
 * @brief 设置映射表项
 * 
 * @param physical_addr 物理地址
 * @param flags 标志位
 * @return uint32_t 设置后的映射表项
 */
uint32_t set_mapping_entry(uint32_t physical_addr, uint32_t flags);

#endif // HMSYS_CACHE_LAYER_H
