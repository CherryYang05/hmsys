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

static void fatal(const char *msg, const char *msg1) {
    fprintf(stderr, "ERROR: %s%s\n", msg, msg1 ? msg1 : "");
    exit(1);
}

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

    // 2. 转化表
    toml_table_t *cache = toml_table_in(conf, "cache");
    if (!cache) {
        fatal("missing [server]", "");
    }

    // 3. 提取值
    toml_datum_t size = toml_int_in(cache, "size");
    if (!size.ok) {
        fatal("cannot read cache.size", "");
    }

    toml_datum_t assoc = toml_int_in(cache, "assoc");
    if (!assoc.ok) {
        fatal("cannot read cache.assoc", "");
    }

    printf("size: %d\n", size.u.b);
    printf("assoc: %d\n", assoc.u.b);

    // 4. 释放内存
    toml_free(conf);
}

void hmsys_setup() {
    hmsys_read_config(CONFIG_PATH);
}
