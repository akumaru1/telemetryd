#define _POSIX_C_SOURCE 200809L
#include "sysfs_ingest.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#define PATH_MAX 4096


#define SYSFS_PATH "/sys/class/hwmon"

static void find_cpu_temp_path(char *resolved_path, size_t max_len) {
    resolved_path[0] = '\0';

    DIR *dir = opendir(SYSFS_PATH);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    char best_path[PATH_MAX] = "";
    int found_best = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) == 0) {
            char name_path[PATH_MAX];
            snprintf(name_path, sizeof(name_path), "%s/%s/name", SYSFS_PATH, entry->d_name);

            int name_fd = open(name_path, O_RDONLY | O_CLOEXEC);
            if (name_fd >= 0) {
                char name_buf[64];
                ssize_t bytes_read = read(name_fd, name_buf, sizeof(name_buf) - 1);
                close(name_fd);

                if (bytes_read > 0) {
                    name_buf[bytes_read] = '\0';
                    // Remove trailing newline
                    name_buf[strcspn(name_buf, "\n")] = '\0';

                    // Check for AMD (k10temp) or Intel (coretemp) core sensors
                    if (strcmp(name_buf, "k10temp") == 0 || strcmp(name_buf, "coretemp") == 0) {
                        snprintf(best_path, sizeof(best_path), "%s/%s/temp1_input", SYSFS_PATH, entry->d_name);
                        found_best = 1;
                        break;
                    }
                }
            }
        }
    }
    closedir(dir);

    if (found_best) {
        snprintf(resolved_path, max_len, "%s", best_path);
    }
}

static char resolved_temp_path[PATH_MAX] = "";
static pthread_once_t temp_path_once = PTHREAD_ONCE_INIT;

static void resolve_temp_path_once(void) {
    find_cpu_temp_path(resolved_temp_path, sizeof(resolved_temp_path));
}

int read_cpu_temp(double *temp_celsius) {
    if (!temp_celsius) {
        /* TEMP_ERR_INVALID_ARG: "invalid argument (NULL output pointer)" */
        return TEMP_ERR_INVALID_ARG;
    }

    pthread_once(&temp_path_once, resolve_temp_path_once);

    if (resolved_temp_path[0] == '\0') {
        /* TEMP_ERR_NO_SENSOR: "no supported CPU temp sensor (k10temp/coretemp) found" */
        return TEMP_ERR_NO_SENSOR;
    }

    int fd = open(resolved_temp_path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        /* TEMP_ERR_NO_FILE: "sysfs temperature file not found or not openable" */
        return TEMP_ERR_NO_FILE;
    }

    char buf[32];
    ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (bytes_read <= 0) {
        /* TEMP_ERR_READ_FAILED: "read from sysfs temperature file failed" */
        return TEMP_ERR_READ_FAILED;
    }

    buf[bytes_read] = '\0';

    char *endptr;
    long temp_milli = strtol(buf, &endptr, 10);
    /* endptr == buf: no digits parsed at all. Allow a trailing newline
     * (sysfs files are newline-terminated) but reject anything else. */
    if (endptr == buf || (*endptr != '\0' && *endptr != '\n')) {
        /* TEMP_ERR_PARSE_FAILED: "sysfs temperature value unparsable or out of range" */
        return TEMP_ERR_PARSE_FAILED;
    }

    if (temp_milli < 0 || temp_milli > 150000) {
        /* TEMP_ERR_PARSE_FAILED: "sysfs temperature value unparsable or out of range" */
        return TEMP_ERR_PARSE_FAILED;
    }

    *temp_celsius = temp_milli / 1000.0;
    return 0;
}

const char *sysfs_ingest_strerror(int err) {
    switch (err) {
        case 0:                     return "success";
        case TEMP_ERR_INVALID_ARG:  return "invalid argument (NULL output pointer)";
        case TEMP_ERR_NO_FILE:      return "sysfs temperature file not found or not openable";
        case TEMP_ERR_READ_FAILED:  return "read from sysfs temperature file failed";
        case TEMP_ERR_PARSE_FAILED: return "sysfs temperature value unparsable or out of range";
        case TEMP_ERR_NO_SENSOR:    return "no supported CPU temp sensor (k10temp/coretemp) found";
        default:                    return "unknown error";
    }
}

