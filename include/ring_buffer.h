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

typedef struct {
    telemetry_sample_t *buffer; 
    size_t capacity;            
    size_t head;                
    size_t tail;                
    size_t count;
    pthread_mutex_t lock;      
} ring_buffer_t;


int ring_buffer_init(ring_buffer_t *rb, size_t capacity);

void ring_buffer_destroy(ring_buffer_t *rb);

int ring_buffer_push(ring_buffer_t *rb, telemetry_sample_t sample);

int ring_buffer_pop(ring_buffer_t *rb, telemetry_sample_t *sample);

const char *ring_buffer_strerror(int err);

#endif // RING_BUFFER_H
