#define _POSIX_C_SOURCE 200809L
#include "sysfs_ingest.h"
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define SYSFS_BASE_PATH "/sys/class/hwmon"
#define DEFAULT_TEMP_PATH "/sys/class/hwmon/hwmon0/temp1_input"

static void find_cpu_temp_path(char *resolved_path, size_t max_len) {
    // Default fallback
    snprintf(resolved_path, max_len, "%s", DEFAULT_TEMP_PATH);

    DIR *dir = opendir(SYSFS_BASE_PATH);
    if (!dir) {
        return;
    }

    struct dirent *entry;
    char best_path[PATH_MAX] = "";
    int found_best = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "hwmon", 5) == 0) {
            char name_path[PATH_MAX];
            snprintf(name_path, sizeof(name_path), "%s/%s/name", SYSFS_BASE_PATH, entry->d_name);

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
                        snprintf(best_path, sizeof(best_path), "%s/%s/temp1_input", SYSFS_BASE_PATH, entry->d_name);
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

int read_cpu_temp(double *temp_celsius) {
    if (!temp_celsius) {
        return -1;
    }

    static char temp_path[PATH_MAX] = "";
    static int path_resolved = 0;

    if (!path_resolved) {
        find_cpu_temp_path(temp_path, sizeof(temp_path));
        path_resolved = 1;
    }

    int fd = open(temp_path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        perror("Error opening temperature sysfs file");
        return -2;
    }

    char buf[32];
    ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (bytes_read <= 0) {
        return -3;
    }

    buf[bytes_read] = '\0';
    int temp_milli = atoi(buf);
    *temp_celsius = temp_milli / 1000.0;
    return 0;
}

