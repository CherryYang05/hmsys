/**
 * @Author      : CherryYang
 * @Date        : 2025-01-13 21:00:36
 * @FilePath    : /hmsys/src/log.h
 * @Description : 日志打印头文件
 */
#ifndef HMSYS_LOG_H
#define HMSYS_LOG_H

#define INFO                INFO
#define INFO_B              INFO_BORDER
#define DEBUG               DEBUG
#define DEBUG_B             DEBUG_BORDER
#define WARNING             WARNING
#define WARNING_B           WARNING_BORDER
#define ERROR               ERROR
#define ERROR_B             ERROR_BORDER

#define LOG_ERROR_STYLE             "\x1b[31m"      // 红色
#define LOG_ERROR_BORDER_STYLE      "\x1b[31;1m"    // 红色加粗
#define LOG_INFO_STYLE              "\x1b[32m"      // 绿色
#define LOG_INFO_BORDER_STYLE       "\x1b[32;1m"    // 绿色加粗
#define LOG_WARNING_STYLE           "\x1b[33m"      // 黄色
#define LOG_WARNING_BORDER_STYLE    "\x1b[33;1m"    // 黄色加粗
#define LOG_DEBUG_STYLE             "\x1b[34m"      // 蓝色
#define LOG_DEBUG_BORDER_STYLE      "\x1b[34;1m"    // 蓝色加粗
#define LOG_STYLE_CLEAR             "\x1b[0m "      // 清除样式

#define LOG_ERROR(...)      printf(LOG_ERROR_STYLE "[ERROR]" LOG_STYLE_CLEAR __VA_ARGS__)
#define LOG_ERROR_BORDER(...) printf(LOG_ERROR_BORDER_STYLE "[ERROR]" LOG_STYLE_CLEAR __VA_ARGS__)

#define LOG_INFO(...)       printf(LOG_INFO_STYLE "[INFO] " LOG_STYLE_CLEAR __VA_ARGS__)
#define LOG_INFO_BORDER(...) printf(LOG_INFO_BORDER_STYLE "[INFO] " LOG_STYLE_CLEAR __VA_ARGS__)

#define LOG_WARNING(...)    printf(LOG_WARNING_STYLE "[WARN] " LOG_STYLE_CLEAR __VA_ARGS__)
#define LOG_WARNING_BORDER(...) printf(LOG_WARNING_BORDER_STYLE "[WARN] " LOG_STYLE_CLEAR __VA_ARGS__)

#define LOG_DEBUG(...)      printf(LOG_DEBUG_STYLE "[DEBUG]" LOG_STYLE_CLEAR __VA_ARGS__)
#define LOG_DEBUG_BORDER(...) printf(LOG_DEBUG_BORDER_STYLE "[DEBUG]" LOG_STYLE_CLEAR __VA_ARGS__)

#define LOG(TYPE, ...)      LOG_##TYPE(__VA_ARGS__)

#endif