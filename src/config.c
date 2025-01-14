/**
 * @Author      : CherryYang
 * @Date        : 2025-01-12 17:57:27
 * @FilePath    : /hmsys/src/config.c
 * @Description : 读取 toml 配置文件
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "toml.h"
#include "stats.h"
#include "log.h"
#include "trace.h"

FILE *statsfile = NULL;
FILE *response = NULL;
FILE *tracefile = NULL;

/**
 * @brief 读取并解析 toml 配置文件
 * 
 * @param config_file 
 */
static void hmsys_read_config(const char *config_file) {
    FILE *fp;
    char errbuf[200];

    // 1. 读取配置文件
    fp = fopen(config_file, "r");
    if (!fp) {
        fatal("cannot open config.toml - ", strerror(errno));
    }

    toml_table_t *conf = toml_parse_file(fp, errbuf, sizeof(errbuf));
    fclose(fp);

    if (!conf) {
        fatal("cannot parse - ", errbuf);
    }

    // 2. 转化表单
    toml_table_t *cache = toml_table_in(conf, "cache");
    if (!cache) {
        fatal("missing [server]", "");
    }

    // 3. 提取值
    toml_datum_t size = toml_int_in(cache, "block_size");
    if (!size.ok) {
        fatal("cannot read cache.size", "");
    }

    toml_datum_t assoc = toml_int_in(cache, "assoc");
    if (!assoc.ok) {
        fatal("cannot read cache.assoc", "");
    }

    toml_datum_t policy = toml_int_in(cache, "cache_policy");
    if (!policy.ok) {
        fatal("cannot read cache.policy", "");
    }

    printf("size: %d\n", size.u.b);
    printf("assoc: %d\n", assoc.u.b);

    // 4. 释放内存
    toml_free(conf);
}

void hmsys_setup() {
    hmsys_read_config(CONFIG_PATH);

    statsfile = fopen(STATS_PATH, "w+");
    if (!statsfile) {
        fatal("Can not open stats file - ", strerror(errno));
    }

    response = fopen(RESPONSE_PATH, "w+");
    if (!response) {
        fatal("Can not open response file - ", strerror(errno));
    }

    tracefile = fopen(TRACE_PATH, "r");
    if (!tracefile) {
        fatal("Can not open trace file - ", strerror(errno));
    }

    ioreq_event *io_event;
    io_event = (ioreq_event *)malloc(sizeof(ioreq_event));

    hmsys_iotrace_get_ioreq_event(tracefile, io_event);

    LOG(INFO, "hmsys init done.\n");

}
