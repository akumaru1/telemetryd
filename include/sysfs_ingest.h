#ifndef SYSFS_INGEST_H
#define SYSFS_INGEST_H

/* Error codes returned by read_cpu_temp() */
#define TEMP_ERR_INVALID_ARG   (-1) /* temp_celsius was NULL */
#define TEMP_ERR_NO_FILE       (-2) /* sysfs temp file could not be opened */
#define TEMP_ERR_READ_FAILED   (-3) /* read() from sysfs temp file failed */
#define TEMP_ERR_PARSE_FAILED  (-4) /* sysfs contents were not a valid reading */
#define TEMP_ERR_NO_SENSOR     (-5) /* no supported hwmon CPU temp sensor found */

int read_cpu_temp(double *temp_celsius);

const char *sysfs_ingest_strerror(int err);

#endif // SYSFS_INGEST_H
