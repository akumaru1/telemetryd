#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "telemetry_sample.h"
#include <stddef.h>
#include <pthread.h>

/**
 * @brief Circular ring buffer structure for storing telemetry samples.
 */
typedef struct {
    telemetry_sample_t *buffer; /**< Dynamic array of telemetry samples */
    size_t capacity;            /**< Maximum capacity of the buffer (must be a power of two) */
    size_t head;                /**< Index of the oldest sample (read pointer) */
    size_t tail;                /**< Index of the next available sample slot (write pointer) */
    size_t size;                /**< Current number of samples stored in the buffer */
    pthread_mutex_t lock;       /**< Mutex lock for thread-safety */
    pthread_cond_t not_empty;   /**< Condition variable signaling buffer is not empty */
    pthread_cond_t not_full;    /**< Condition variable signaling buffer is not full */
} ring_buffer_t;

/**
 * @brief Initializes the ring buffer with the specified capacity.
 * 
 * @param rb Pointer to the ring_buffer_t struct to initialize.
 * @param capacity The number of telemetry samples the buffer can hold (must be a power of two).
 * @return int 0 on success, negative value on error.
 */
int ring_buffer_init(ring_buffer_t *rb, size_t capacity);

/**
 * @brief Cleans up resources allocated for the ring buffer.
 * 
 * @param rb Pointer to the ring_buffer_t struct to destroy.
 */
void ring_buffer_destroy(ring_buffer_t *rb);

/**
 * @brief Thread-safely pushes a sample into the circular buffer. Overwrites the oldest sample if full.
 * 
 * @param rb Pointer to the ring_buffer_t struct.
 * @param sample The telemetry sample to push.
 * @return int 0 on success, negative value on error.
 */
int ring_buffer_push(ring_buffer_t *rb, telemetry_sample_t sample);

/**
 * @brief Thread-safely pops a sample from the circular buffer (non-blocking).
 * 
 * @param rb Pointer to the ring_buffer_t struct.
 * @param sample Pointer to telemetry_sample_t where the popped sample will be stored.
 * @return int 0 on success, negative value on error (e.g. if buffer is empty).
 */
int ring_buffer_pop(ring_buffer_t *rb, telemetry_sample_t *sample);

#endif // RING_BUFFER_H
