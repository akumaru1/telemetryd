#include "procfs_ingest.h"
#include <stdio.h>
#include <string.h>

int read_ram_utilization(double *ram_utilization) {
    if (!ram_utilization) {
        return -1;
    }

    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("Error opening /proc/meminfo");
        return -2;
    }

    char line[256];
    unsigned long mem_total = 0;
    unsigned long mem_free = 0;
    unsigned long mem_buffers = 0;
    unsigned long mem_cached = 0;
    unsigned long mem_available = 0;
    int has_available = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line + 9, "%lu", &mem_total);
        } else if (strncmp(line, "MemFree:", 8) == 0) {
            sscanf(line + 8, "%lu", &mem_free);
        } else if (strncmp(line, "Buffers:", 8) == 0) {
            sscanf(line + 8, "%lu", &mem_buffers);
        } else if (strncmp(line, "Cached:", 7) == 0) {
            sscanf(line + 7, "%lu", &mem_cached);
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line + 13, "%lu", &mem_available);
            has_available = 1;
        }
    }
    fclose(fp);

    if (mem_total == 0) {
        return -3;
    }

    if (!has_available) {
        mem_available = mem_free + mem_buffers + mem_cached;
    }

    if (mem_available > mem_total) {
        mem_available = mem_total;
    }

    *ram_utilization = ((double)(mem_total - mem_available) / mem_total) * 100.0;
    return 0;
}
