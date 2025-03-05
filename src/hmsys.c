#include "config.h"
#include "driver.h"
#include "stats.h"
#include "toml.h"
#include "global.h"
#include "log.h"
#include "cache.h"
#include "trace.h"
#include "cache_layer.h"



static void sys_run() {
    // 创建并初始化缓存设备
    struct cache_dev *cache = cache_create_and_init();
    if (!cache) {
        fatal("Failed to create and initialize cache device", "");
        return;
    }
    
    // 初始化地址映射表
    address_mapping_t *mapping = init_address_mapping();
    if (!mapping) {
        fatal("Failed to initialize address mapping", "");
        cache_free_memory(cache);
        return;
    }
    
    LOG(INFO, "Address mapping initialized successfully\n");
    
    // 读取并处理所有trace
    while (1) {
        // 读取一条trace
        ioreq_event *new_event = (ioreq_event *)malloc(sizeof(ioreq_event));
        if (!new_event) {
            fatal("Failed to allocate memory for IO event", "");
            break;
        }
        
        // 通过trace模块获取IO请求
        ioreq_event *event = hmsys_iotrace_get_ioreq_event(tracefile, new_event);
        if (event == NULL) {
            // 没有更多的trace，释放资源并退出循环
            free(new_event);
            break;
        }
        
        LOG(INFO, "Processing trace - blkno: %ld, bcount: %ld, flags: %s\n", 
            event->blkno, event->bcount, (event->flags == READ) ? "READ" : "WRITE");
        
        // 创建地址映射
        int mapping_result = create_mapping(mapping, event);
        if (mapping_result != 0) {
            LOG(ERROR, "Failed to create mapping for request - blkno: %ld, bcount: %ld, flags: %s\n", 
                event->blkno, event->bcount, (event->flags == READ) ? "READ" : "WRITE");
            // 对于读请求，如果映射失败，可能是因为读取未映射的区域，这是正常的
            if (event->flags == WRITE) {
                fatal("Failed to create mapping for write request", "");
            }
        }
        
        // 通过缓存管理模块处理请求
        hmsys_cache_access(cache, event, mapping);
        
        // 处理完成后释放资源
        free(event);
    }
    
    // 打印映射表状态
    print_mapping_status(mapping);
    
    // 释放资源
    free_address_mapping(mapping);
    cache_free_memory(cache);

    fclose(tracefile);
}

void fatal(const char *msg, const char *msg1) {
    // fprintf(stderr, "ERROR: %s%s\n", msg, msg1 ? msg1 : "");
    LOG(ERROR, "%s%s\n", msg, msg1 ? msg1 : "");
    exit(1);
}

int main() {

    time_filter = (struct timefilter_t *)(calloc(1, sizeof(struct timefilter_t)));
    time_t startTime, endTime;
    static struct timeval initStartTime;
    static struct timeval initEndTime;


    time(&time_filter->start_time);

    hmsys_setup();

    sys_run();

    hmsys_cleanup();

    hmsys_stats();

    time(&time_filter->end_time);
    free(time_filter);

    return 0;
}
