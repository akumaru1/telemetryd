#ifndef PROCFS_INGEST_H
#define PROCFS_INGEST_H

/**
 * @brief Reads the RAM utilization percentage from /proc/meminfo.
 * 
 * @param ram_utilization Pointer to double where the percentage (0.0 to 100.0) will be stored.
 * @return int 0 on success, negative value on error.
 */
int read_ram_utilization(double *ram_utilization);

#endif // PROCFS_INGEST_H
