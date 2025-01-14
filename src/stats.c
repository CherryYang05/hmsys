/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 18:16:05
 * @FilePath    : /hmsys/src/stats.c
 * @Description : 输出统计数据
 */

#include "stats.h"

void hmsys_cleanup() {

    fclose(statsfile);
    fclose(response);

}


void hmsys_stats() {
    
}