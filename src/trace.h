/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:13:40
 * @FilePath    : /hmsys/src/trace.h
 * @Description : 读取 trace 工作负载的头文件
 */

#ifndef       HMSYS_TRACE_H
#define       HMSYS_TRACE_H

#include <stdio.h>
#include "global.h"
#include "driver.h"

extern FILE *tracefile;

ioreq_event *hmsys_iotrace_get_ioreq_event(FILE *tracefile, ioreq_event *new_event);

#endif