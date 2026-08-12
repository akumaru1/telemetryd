#ifndef TELEMETRY_SAMPLE_H
#define TELEMETRY_SAMPLE_H

#include <time.h>

/**
 * @brief Represents a single telemetry data point.
 */
typedef struct {
    time_t timestamp;        /**< Epoch timestamp of when the telemetry was sampled */
    double cpu_temp;        /**< CPU temperature in Celsius */
    double ram_utilization; /**< RAM utilization percentage (0.0 to 100.0) */
} telemetry_sample_t;

#endif // TELEMETRY_SAMPLE_H
