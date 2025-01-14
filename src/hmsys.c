#include "config.h"
#include "driver.h"
#include "stats.h"
#include "toml.h"
#include "global.h"
#include "log.h"



static void sys_run() {

    // iotrace_validate_get_ioreq_event();
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

    return 0;
}
