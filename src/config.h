/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:12:29
 * @FilePath    : /hmsys/src/config.h
 * @Description : 读取 toml 配置文件的头文件
 */
#ifndef       HMSYS_CONFIG_H      
#define       HMSYS_CONFIG_H

#include "global.h"
#include "cache.h"

extern FILE *statsfile;
extern FILE *response;
extern FILE *tracefile;

/**
 * @brief 获取缓存配置参数
 * 
 * @param cache 缓存设备指针
 */
void get_cache_config(struct cache_dev *cache);

// void hmsys_read_config(const char*);

void hmsys_setup();

#endif