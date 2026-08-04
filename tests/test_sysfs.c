#include "sysfs_ingest.h"
#include <stdio.h>

int main(void) {
    double temp = 0.0;
    int res = read_cpu_temp(&temp);
    if (res == 0) {
        printf("Successfully read CPU temperature: %.3f C\n", temp);
        return 0;
    } else {
        printf("Failed to read CPU temperature, error code: %d\n", res);
        return 1;
    }
}
