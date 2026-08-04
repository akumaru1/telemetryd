#ifndef SYSFS_INGEST_H
#define SYSFS_INGEST_H

/**
 * @brief Reads the CPU temperature from the sysfs path.
 * 
 * @param temp_celsius Pointer to double where the Celsius temperature will be stored.
 * @return int 0 on success, negative value on error.
 */
int read_cpu_temp(double *temp_celsius);

#endif // SYSFS_INGEST_H
