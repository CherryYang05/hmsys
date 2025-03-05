/**
 * @Author      : CherryYang
 * @Date        : 2025-01-24 15:55:48
 * @FilePath    : /hmsys/src/cache_layer.c
 * @Description : 缓存模块，包含地址映射层的实现
 */

#include "cache_layer.h"
#include "log.h"
#include <string.h>

/**
 * @brief 初始化地址映射表
 * 
 * @return address_mapping_t* 初始化的映射表指针，失败返回NULL
 */
address_mapping_t* init_address_mapping() {
    address_mapping_t *mapping = (address_mapping_t *)malloc(sizeof(address_mapping_t));
    if (!mapping) {
        LOG(ERROR, "Failed to allocate memory for address mapping\n");
        return NULL;
    }

    // 分配映射表内存
    mapping->table = (mapping_entry_t *)calloc(MAPPING_TABLE_SIZE, sizeof(mapping_entry_t));
    if (!mapping->table) {
        LOG(ERROR, "Failed to allocate memory for mapping table\n");
        free(mapping);
        return NULL;
    }

    // 初始化映射表
    mapping->next_tape_addr = 0;  // 磁带物理地址从0开始
    mapping->entry_count = 0;

    LOG(INFO, "Address mapping initialized: table size = %u entries\n", MAPPING_TABLE_SIZE);
    return mapping;
}

/**
 * @brief 释放地址映射表
 * 
 * @param mapping 映射表指针
 */
void free_address_mapping(address_mapping_t *mapping) {
    if (mapping) {
        if (mapping->table) {
            free(mapping->table);
        }
        free(mapping);
    }
}

/**
 * @brief 获取映射表项的物理地址部分
 * 
 * @param entry 映射表项
 * @return uint32_t 物理地址
 */
uint32_t get_physical_addr(uint32_t entry) {
    // 高27位为物理地址
    return (entry >> 5) & 0x7FFFFFF; // 27位掩码: 0x7FFFFFF
}

/**
 * @brief 获取映射表项的标志位
 * 
 * @param entry 映射表项
 * @return uint32_t 标志位
 */
uint32_t get_flags(uint32_t entry) {
    // 低2位为标志位
    return entry & 0x03; // 2位掩码: 0x03
}

/**
 * @brief 设置映射表项
 * 
 * @param phy_addr 物理地址
 * @param flags 标志位
 * @return uint32_t 设置后的映射表项
 */
uint32_t set_mapping_entry(uint32_t phy_addr, uint32_t flags) {
    // 高27位为物理地址，低2位为标志位
    return ((phy_addr & 0x7FFFFFF) << 5) | (flags & 0x03);
}

/**
 * @brief 从逻辑地址中提取映射表索引
 * 
 * @param log_addr 逻辑地址
 * @return uint32_t 映射表索引
 */
uint32_t get_mapping_index(uint64_t log_addr) {
    // 从逻辑地址中提取高27位作为映射表索引
    return (uint32_t)((log_addr >> MIN_UNIT_SHIFT) & 0x7FFFFFF);
}

/**
 * @brief 将逻辑地址映射到磁带物理地址
 * 
 * @param mapping 映射表指针
 * @param log_addr 逻辑地址
 * @param is_write 是否为写操作
 * @return uint32_t 映射后的物理地址
 */
uint32_t map_log_to_phy(address_mapping_t *mapping, uint64_t log_addr, int is_write) {
    uint32_t index = get_mapping_index(log_addr);
    uint32_t phy_addr;
    uint32_t flags;
    
    // 检查索引是否有效
    if (index >= MAPPING_TABLE_SIZE) {
        LOG(ERROR, "Invalid mapping index: %u\n", index);
        return (uint32_t)-1;
    }
    
    // 检查映射表项是否有效
    if (!(mapping->table[index].entry & MAP_VALID)) {
        // 如果是写操作，创建新的映射
        if (is_write) {
            // 分配新的磁带物理地址
            phy_addr = mapping->next_tape_addr++;
            flags = MAP_VALID | MAP_DIRTY;
            
            // 更新映射表项
            mapping->table[index].entry = set_mapping_entry(phy_addr, flags);
            mapping->entry_count++;
            
            LOG(DEBUG, "Created new mapping: logical=%lu (index=%u) -> physical=%u, flags=0x%x\n", 
                log_addr, index, phy_addr, flags);
        } else {
            // 读操作但映射不存在，返回错误
            LOG(ERROR, "Read operation on unmapped address: %lu (index=%u)\n", 
                log_addr, index);
            
            // 返回错误码
            return (uint32_t)-1;
        }
    } else {
        /*
            trace 保证不会出现覆盖写操作，因此该种情况不会存在
        */
        // 映射已存在
        phy_addr = get_physical_addr(mapping->table[index].entry);
        flags = get_flags(mapping->table[index].entry);
        
        // 如果是写操作，设置修改位
        if (is_write && !(flags & MAP_DIRTY)) {
            flags |= MAP_DIRTY;
            mapping->table[index].entry = set_mapping_entry(phy_addr, flags);
            
            LOG(DEBUG, "Updated mapping flags: logical=%lu (index=%u) -> physical=%u, flags=0x%x\n", 
                log_addr, index, phy_addr, flags);
        }
    }
    
    return phy_addr;
}

/**
 * @brief 处理IO请求，创建映射关系
 * 
 * @param mapping 映射表指针
 * @param req IO请求
 * @return int 成功返回0，失败返回-1
 */
int create_mapping(address_mapping_t *mapping, ioreq_event *req) {
    if (!mapping || !req) {
        LOG(ERROR, "Invalid parameters for create_mapping\n");
        return -1;
    }
    
    // 计算请求的起始和结束逻辑地址
    uint64_t start_addr = req->blkno * BLOCK_SIZE;
    uint64_t end_addr = start_addr + req->bcount * BLOCK_SIZE - 1;
    
    // 计算起始和结束的最小存储单位地址
    uint64_t start_unit = start_addr / MIN_UNIT_SIZE;
    uint64_t end_unit = end_addr / MIN_UNIT_SIZE;
    
    int is_write = (req->flags == WRITE);
    
    LOG(INFO, "Processing request: blkno=%ld, bcount=%ld, flags=%s\n", 
        req->blkno, req->bcount, is_write ? "WRITE" : "READ");
    LOG(DEBUG, "Address range: %lu - %lu, Units: %lu - %lu\n", 
        start_addr, end_addr, start_unit, end_unit);
    
    // 为每个最小存储单位创建映射
    for (uint64_t unit = start_unit; unit <= end_unit; unit++) {
        uint64_t unit_addr = unit * MIN_UNIT_SIZE;
        uint32_t phy_addr = map_log_to_phy(mapping, unit_addr, is_write);
        
        if (phy_addr == (uint32_t)-1) {
            LOG(ERROR, "Failed to map logical address: %lu\n", 
                unit_addr);
            return -1;
        }
        
        LOG(DEBUG, "Mapped unit %lu: logical=%lu -> physical=%u\n", 
            unit, unit_addr, phy_addr);
    }
    
    return 0;
}

/**
 * @brief 打印映射表状态
 * 
 * @param mapping 映射表指针
 */
void print_mapping_status(address_mapping_t *mapping) {
    if (!mapping) {
        LOG(ERROR, "Invalid mapping pointer\n");
        return;
    }
    
    uint32_t valid_count = 0;
    uint32_t dirty_count = 0;
    
    // 统计有效和脏的映射表项
    for (uint32_t i = 0; i < MAPPING_TABLE_SIZE; i++) {
        uint32_t flags = get_flags(mapping->table[i].entry);
        if (flags & MAP_VALID) {
            valid_count++;
            if (flags & MAP_DIRTY) {
                dirty_count++;
            }
        }
    }
    
    LOG(DEBUG, "Mapping table status:\n");
    LOG(DEBUG, "  Total entries: %u\n", MAPPING_TABLE_SIZE);
    LOG(DEBUG, "  Used entries: %u (%.2f%%)\n", 
        mapping->entry_count, (float)mapping->entry_count / MAPPING_TABLE_SIZE * 100);
    LOG(DEBUG, "  Valid entries: %u (%.2f%%)\n", 
        valid_count, (float)valid_count / MAPPING_TABLE_SIZE * 100);
    LOG(DEBUG, "  Dirty entries: %u (%.2f%%)\n", 
        dirty_count, (float)dirty_count / MAPPING_TABLE_SIZE * 100);
    LOG(DEBUG, "  Next tape address: %u\n", 
        mapping->next_tape_addr);
}


 