#include "procfs_ingest.h"
#include <stdio.h>
#include <string.h>

#define PROCFS_MEMINFO_PATH "/proc/meminfo"

int read_ram_utilization(double *ram_utilization) {
    if (!ram_utilization) {
        /* RAM_ERR_INVALID_ARG: "invalid argument (NULL output pointer)" */
        return RAM_ERR_INVALID_ARG;
    }

    FILE *fp = fopen(PROCFS_MEMINFO_PATH, "r");
    if (!fp) {
        /* RAM_ERR_NO_FILE: "/proc/meminfo not found or not openable" */
        return RAM_ERR_NO_FILE;
    }

    char line[256];
    unsigned long mem_total = 0;
    unsigned long mem_available = 0;
    int has_total = 0;
    int has_available = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            if (sscanf(line + 9, "%lu", &mem_total) == 1) {
                has_total = 1;
            }
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            if (sscanf(line + 13, "%lu", &mem_available) == 1) {
                has_available = 1;
            }
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        /* RAM_ERR_READ_FAILED: "read from /proc/meminfo failed" */
        return RAM_ERR_READ_FAILED;
    }
    fclose(fp);

    if (!has_total || mem_total == 0) {
        /* RAM_ERR_PARSE_FAILED: "/proc/meminfo missing or invalid MemTotal field" */
        return RAM_ERR_PARSE_FAILED;
    }

    if (!has_available) {
        /* RAM_ERR_NO_AVAILABLE: "/proc/meminfo missing MemAvailable field" */
        return RAM_ERR_NO_AVAILABLE;
    }

    if (mem_available > mem_total) {
        mem_available = mem_total;
    }

    *ram_utilization = ((double)(mem_total - mem_available) / mem_total) * 100.0;
    return 0;
}

const char *procfs_ingest_strerror(int err) {
    switch (err) {
        case 0:                    return "success";
        case RAM_ERR_INVALID_ARG:  return "invalid argument (NULL output pointer)";
        case RAM_ERR_NO_FILE:      return "/proc/meminfo not found or not openable";
        case RAM_ERR_READ_FAILED:  return "read from /proc/meminfo failed";
        case RAM_ERR_PARSE_FAILED: return "/proc/meminfo missing or invalid MemTotal field";
        case RAM_ERR_NO_AVAILABLE: return "/proc/meminfo missing MemAvailable field";
        default:                   return "unknown error";
    }
}
