#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "telemetry_sample.h"
#include <stddef.h>

/**
 * @brief Circular ring buffer structure for storing telemetry samples.
 */
typedef struct {
    telemetry_sample_t *buffer; /**< Dynamic array of telemetry samples */
    size_t capacity;            /**< Maximum capacity of the buffer */
    size_t head;                /**< Index of the oldest sample (read pointer) */
    size_t tail;                /**< Index of the next available sample slot (write pointer) */
    size_t size;                /**< Current number of samples stored in the buffer */
} ring_buffer_t;

/**
 * @brief Initializes the ring buffer with the specified capacity.
 * 
 * @param rb Pointer to the ring_buffer_t struct to initialize.
 * @param capacity The number of telemetry samples the buffer can hold.
 * @return int 0 on success, negative value on error.
 */
int ring_buffer_init(ring_buffer_t *rb, size_t capacity);

/**
 * @brief Cleans up resources allocated for the ring buffer.
 * 
 * @param rb Pointer to the ring_buffer_t struct to destroy.
 */
void ring_buffer_destroy(ring_buffer_t *rb);

#endif // RING_BUFFER_H
