#include "procfs_ingest.h"
#include <stdio.h>

int main(void) {
    double ram_util = 0.0;
    int res = read_ram_utilization(&ram_util);
    if (res == 0) {
        printf("Successfully read RAM utilization: %.3f%%\n", ram_util);
        return 0;
    } else {
        printf("Failed to read RAM utilization, error code: %d\n", res);
        return 1;
    }
}
