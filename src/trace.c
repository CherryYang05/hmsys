/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:13:20
 * @FilePath    : /hmsys/src/trace.c
 * @Description : 读取 trace 工作负载
 */
#include "trace.h"

#include "driver.h"
#include "global.h"
#include "log.h"

ioreq_event *hmsys_iotrace_get_ioreq_event(FILE *tracefile, ioreq_event *new_event) {
    char line[201];
    char rw;
    double nextinter;

    if (fgets(line, 200, tracefile) == NULL) {
        // addtoextraq((event *)new_event);
        return (NULL);
    }


#ifdef INFO
    LOG(INFO, "%s", line);
#endif

    if (sscanf(line, "%c %ld %ld %lf\n",
               &rw,                         // R/W
               &new_event->blkno,           // 单位：扇区
               &new_event->bcount,          // 单位：扇区
               &nextinter) != 4) {          // 距离下一个 trace 的时间间隔，单位 ms
        fatal("Wrong number of arguments for I/O trace event type\n", "");
    }

    if (rw == 'R') {
        new_event->flags = READ;
    } else if (rw == 'W') {
        new_event->flags = WRITE;
    } else {
        fatal("Invalid R/W value: ", &rw);
    }

    // if (first_arrived == 1) {
    //     new_event->arrivetime = 0;
    //     disksim->sum_arrive_time = validate_nextinter;
    //     first_arrived = 0;
    // } else {
    //     new_event->arrivetime = disksim->sum_arrive_time;
    //     disksim->sum_arrive_time = new_event->arrivetime + validate_nextinter;
    // }


    new_event->isPending = 0;
    new_event->opid = 0;
    new_event->cause = 0;
    new_event->busno = 0;
    return (new_event);
}