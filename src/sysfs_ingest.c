#include "sysfs_ingest.h"
#include <stdio.h>
#include <stdlib.h>

#define SYSFS_TEMP_PATH "/sys/class/hwmon/hwmon0/temp1_input"

int read_cpu_temp(double *temp_celsius) {
    if (!temp_celsius) {
        return -1;
    }

    FILE *fp = fopen(SYSFS_TEMP_PATH, "r");
    if (!fp) {
        perror("Error opening temperature sysfs file");
        return -2;
    }

    int temp_milli = 0;
    if (fscanf(fp, "%d", &temp_milli) != 1) {
        fclose(fp);
        return -3;
    }

    fclose(fp);
    *temp_celsius = temp_milli / 1000.0;
    return 0;
}
