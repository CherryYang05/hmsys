#include "config.h"
#include "driver.h"
#include "stats.h"
#include "toml.h"


static void sys_run() {

    // iotrace_validate_get_ioreq_event();
} 

int main() {

    hmsys_setup();

    sys_run();

    hmsys_cleanup();

    hmsys_stats();

    return 0;
}
