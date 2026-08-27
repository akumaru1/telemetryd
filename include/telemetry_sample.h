#ifndef TELEMETRY_SAMPLE_H
#define TELEMETRY_SAMPLE_H

#include <time.h>

typedef struct {
    time_t timestamp;       
    double cpu_temp;        
    double ram_utilization; 
} telemetry_sample_t;

#endif 
