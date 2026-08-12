#include "sysfs_ingest.h"
#include "procfs_ingest.h"
#include "telemetry_sample.h"
#include <stdio.h>
#include <time.h>

int main(void) {
    telemetry_sample_t sample;
    
    // Get current timestamp
    sample.timestamp = time(NULL);
    
    // Read CPU temperature
    int res_cpu = read_cpu_temp(&sample.cpu_temp);
    if (res_cpu != 0) {
        printf("Failed to read CPU temperature, error: %d\n", res_cpu);
        return 1;
    }
    
    // Read RAM utilization
    int res_ram = read_ram_utilization(&sample.ram_utilization);
    if (res_ram != 0) {
        printf("Failed to read RAM utilization, error: %d\n", res_ram);
        return 1;
    }
    
    // Print telemetry sample details
    printf("Successfully populated telemetry_sample_t:\n");
    printf("  Timestamp:       %ld\n", (long)sample.timestamp);
    printf("  CPU Temperature: %.3f C\n", sample.cpu_temp);
    printf("  RAM Utilization: %.3f%%\n", sample.ram_utilization);
    
    return 0;
}
