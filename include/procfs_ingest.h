#ifndef PROCFS_INGEST_H
#define PROCFS_INGEST_H

/* Returned by read_ram_utilization() */
#define RAM_ERR_INVALID_ARG   (-1) /* ram_utilization was NULL */
#define RAM_ERR_NO_FILE       (-2) /* /proc/meminfo could not be opened */
#define RAM_ERR_READ_FAILED   (-3) /* read() from /proc/meminfo failed */
#define RAM_ERR_PARSE_FAILED  (-4) /* /proc/meminfo missing or invalid MemTotal field */
#define RAM_ERR_NO_AVAILABLE  (-5) /* /proc/meminfo missing MemAvailable field */

int read_ram_utilization(double *ram_utilization);

const char *procfs_ingest_strerror(int err);

#endif 