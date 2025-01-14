/**
 * @Author      : CherryYang
 * @Date        : 2025-01-13 22:07:44
 * @FilePath    : /hmsys/src/global.h
 * @Description : 全局头文件、结构体和宏定义
 */

#ifndef HMSYS_GLOBAL_H
#define HMSYS_GLOBAL_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define READ 0
#define WRITE 1

// 定义系统最小粒度大小
#define BLOCK_SIZE 512

/**
 * @brief 该结构体用来统计模拟器各个部分的时间
 *
 */
typedef struct timefilter_t {
    time_t start_time;
    time_t end_time;
    double total_time;             // 整个系统运行时间
    double init_time;              // 初始化时间
    double stats_time;             // 数据统计时间，即打印每条 trace 时延到输出文件中的时间
    double event_type_time[320];  // 分别统计不同状态的事件所花费的时间
} timefilter;

timefilter *time_filter;


void fatal(const char *msg, const char *msg1);

#endif
