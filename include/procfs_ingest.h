#ifndef PROCFS_INGEST_H
#define PROCFS_INGEST_H

/* Error codes returned by read_ram_utilization() */
#define RAM_ERR_INVALID_ARG   (-1) /* ram_utilization was NULL */
#define RAM_ERR_NO_FILE       (-2) /* /proc/meminfo could not be opened */
#define RAM_ERR_READ_FAILED   (-3) /* read() from /proc/meminfo failed */
#define RAM_ERR_PARSE_FAILED  (-4) /* /proc/meminfo missing or invalid MemTotal field */

/**
 * @brief Reads the RAM utilization percentage from /proc/meminfo.
 *
 * @param ram_utilization Pointer to double where the percentage (0.0 to 100.0) will be stored.
 * @return int 0 on success, negative RAM_ERR_* value on error.
 */
int read_ram_utilization(double *ram_utilization);

/**
 * @brief Translates a read_ram_utilization() return code into a human-readable string.
 *
 * @param err The int returned by read_ram_utilization().
 * @return const char* Static, non-NULL description (never needs freeing).
 */
const char *procfs_ingest_strerror(int err);

#endif // PROCFS_INGEST_H
