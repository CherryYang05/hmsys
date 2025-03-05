/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:13:56
 * @FilePath    : /hmsys/src/driver.c
 * @Description : driver 层逻辑，包含将事件转化为请求，请求切分，请求发送等逻辑
 */

 #include "trace.h"
 #include "log.h"

void iotrace_validate_get_ioreq_event(FILE *tracefile) {
    ioreq_event *new_event = (ioreq_event *)malloc(sizeof(ioreq_event));
    if (!new_event) {
        LOG(ERROR, "Failed to allocate memory for IO event\n");
        return;
    }
    
    ioreq_event *event = hmsys_iotrace_get_ioreq_event(tracefile, new_event);
    if (event != NULL) {
        LOG(INFO, "Read trace - blkno: %ld, bcount: %ld, flags: %s\n", 
            event->blkno, event->bcount, (event->flags == READ) ? "READ" : "WRITE");
        free(event);
    } else {
        LOG(INFO, "No more traces to read\n");
        free(new_event);
    }
}