#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include "telemetry_sample.h"
#include <stddef.h>
#include <pthread.h>

/* Error codes returned by ring_buffer_init(), ring_buffer_push(), and ring_buffer_pop() */
#define RB_ERR_INVALID_ARG       (-1) /* a required pointer argument was NULL, or capacity was 0 */
#define RB_ERR_ALLOC_FAILED      (-2) /* malloc() for the sample buffer failed (ring_buffer_init) */
#define RB_ERR_MUTEX_INIT_FAILED (-3) /* pthread_mutex_init() failed (ring_buffer_init) */
#define RB_ERR_NOT_INITIALIZED   (-4) /* buffer is not initialized, or has already been destroyed (ring_buffer_push/ring_buffer_pop) */
#define RB_ERR_EMPTY             (-5) /* buffer has no samples to pop (ring_buffer_pop) */

/**
 * @brief Circular ring buffer structure for storing telemetry samples.
 */
typedef struct {
    telemetry_sample_t *buffer; /**< Dynamic array of telemetry samples */
    size_t capacity;            /**< Maximum capacity of the buffer */
    size_t head;                /**< Index of the oldest sample (read pointer) */
    size_t tail;                /**< Index of the next available sample slot (write pointer) */
    size_t size;                /**< Current number of samples stored in the buffer */
    pthread_mutex_t lock;       /**< Mutex lock for thread-safety */
} ring_buffer_t;

/**
 * @brief Initializes the ring buffer with the specified capacity.
 * 
 * @param rb Pointer to the ring_buffer_t struct to initialize.
 * @param capacity The number of telemetry samples the buffer can hold.
 * @return int 0 on success, negative RB_ERR_* value on error.
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
 * @return int 0 on success, negative RB_ERR_* value on error.
 */
int ring_buffer_push(ring_buffer_t *rb, telemetry_sample_t sample);

/**
 * @brief Thread-safely pops a sample from the circular buffer (non-blocking).
 * 
 * @param rb Pointer to the ring_buffer_t struct.
 * @param sample Pointer to telemetry_sample_t where the popped sample will be stored.
 * @return int 0 on success, negative RB_ERR_* value on error (e.g. RB_ERR_EMPTY if buffer is empty).
 */
int ring_buffer_pop(ring_buffer_t *rb, telemetry_sample_t *sample);

/**
 * @brief Translates a ring_buffer_init()/ring_buffer_push()/ring_buffer_pop() return code into a human-readable string.
 *
 * @param err The int returned by one of those functions.
 * @return const char* Static, non-NULL description (never needs freeing).
 */
const char *ring_buffer_strerror(int err);

#endif // RING_BUFFER_H
