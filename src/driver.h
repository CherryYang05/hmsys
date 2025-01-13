/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:15:00
 * @FilePath    : /hmsys/src/driver.h
 * @Description : driver 层逻辑的头文件
 */

#ifndef HMSYS_DRIVER_H
#define HMSYS_DRIVER_H

typedef struct ioreq_ev {
    double time;
    int type;
    struct ioreq_ev *next;
    struct ioreq_ev *prev;
    int tagID;  // unique value that identifies this IO request;
    long bcount; // 请求的长度（length）
    long blkno; // 请求的偏移地址（offset）
    int flags;
    unsigned int busno;
    unsigned int slotno;
    unsigned int devno;
    int opid;
    int cause;
    int tempint1;
    int tempint2;
    void *tempptr1;
    void *tempptr2;
    void *mems_sled;    /* mems sled associated with a particular event */
    void *mems_reqinfo; /* per-request info for mems subsystem */
    int ssd_elem_num;   /* SSD: element to which this request went */
    int ssd_gang_num;   /* SSD: gang to which this request went */
    double start_time;  /* temporary; used for memulator timing */
    int batchno;
    int batch_complete;
    int batch_size;
    struct ioreq_ev *batch_next;
    struct ioreq_ev *batch_prev;
    double responsetime;
    int uncached;
    int cache_hit;  //lzq
    int act_blk; // 物理块号
    int elem_num; // 物理elem号
    int is_erase; // 新增判断请求是否是擦除，擦除类型的请求不放入flags中。
    double arrivetime; // 请求到达时间=上一个请求到达时间+上一个请求的下一个请求到达间隔
    double pending_time; // 请求等待时间
    int isPending; // 用于判断是否排队
    int is_channel_conflict; // 用于判断排队时是否有channel冲突
    int req_ch;        // 用于记录当前请求被分配到哪个channel上，2024.5.7
    int is_prefetch; // 用于记录是否是预取请求，2024.10.30
    int background_req; // 用于记录是否是后台请求，2024.11.8
} ioreq_event;

void iotrace_validate_get_ioreq_event();


#endif