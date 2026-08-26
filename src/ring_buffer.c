#include "ring_buffer.h"
#include <stdlib.h>

int ring_buffer_init(ring_buffer_t *rb, size_t capacity) {
    if (rb == NULL || capacity == 0) {
        return RB_ERR_INVALID_ARG;
    }

    // Capacity must be a power of two to support fast bitwise wrapping
    if ((capacity & (capacity - 1)) != 0) {
        return RB_ERR_NOT_POWER_OF_TWO;
    }

    rb->buffer = malloc(capacity * sizeof(telemetry_sample_t));
    if (rb->buffer == NULL) {
        return RB_ERR_ALLOC_FAILED;
    }

    if (pthread_mutex_init(&rb->lock, NULL) != 0) {
        free(rb->buffer);
        rb->buffer = NULL;
        return RB_ERR_MUTEX_INIT_FAILED;
    }

    if (pthread_cond_init(&rb->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&rb->lock);
        free(rb->buffer);
        rb->buffer = NULL;
        return RB_ERR_COND_INIT_FAILED;
    }

    if (pthread_cond_init(&rb->not_full, NULL) != 0) {
        pthread_cond_destroy(&rb->not_empty);
        pthread_mutex_destroy(&rb->lock);
        free(rb->buffer);
        rb->buffer = NULL;
        return RB_ERR_COND_INIT_FAILED;
    }
    
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    
    return 0;
}

void ring_buffer_destroy(ring_buffer_t *rb) {
    if (rb == NULL) {
        return;
    }
    
    pthread_mutex_lock(&rb->lock);
    
    if (rb->buffer != NULL) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
    rb->capacity = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
    
    pthread_mutex_unlock(&rb->lock);
    
    pthread_mutex_destroy(&rb->lock);
    pthread_cond_destroy(&rb->not_empty);
    pthread_cond_destroy(&rb->not_full);
}

int ring_buffer_push(ring_buffer_t *rb, telemetry_sample_t sample) {
    if (rb == NULL) {
        return RB_ERR_INVALID_ARG;
    }

    pthread_mutex_lock(&rb->lock);

    if (rb->buffer == NULL) {
        pthread_mutex_unlock(&rb->lock);
        return RB_ERR_NOT_INITIALIZED;
    }

    if (rb->size == rb->capacity) {
        // Buffer is full: overwrite the oldest element at head
        rb->buffer[rb->tail] = sample;
        rb->tail = (rb->tail + 1) & (rb->capacity - 1);
        rb->head = (rb->head + 1) & (rb->capacity - 1); // Discard oldest
    } else {
        // Normal enqueue
        rb->buffer[rb->tail] = sample;
        rb->tail = (rb->tail + 1) & (rb->capacity - 1);
        rb->size++;
    }
    
    pthread_cond_signal(&rb->not_empty);
    pthread_mutex_unlock(&rb->lock);
    
    return 0;
}

int ring_buffer_pop(ring_buffer_t *rb, telemetry_sample_t *sample) {
    if (rb == NULL || sample == NULL) {
        return RB_ERR_INVALID_ARG;
    }

    pthread_mutex_lock(&rb->lock);

    if (rb->buffer == NULL) {
        pthread_mutex_unlock(&rb->lock);
        return RB_ERR_NOT_INITIALIZED;
    }

    if (rb->size == 0) {
        pthread_mutex_unlock(&rb->lock);
        return RB_ERR_EMPTY;
    }
    
    *sample = rb->buffer[rb->head];
    rb->head = (rb->head + 1) & (rb->capacity - 1);
    rb->size--;
    
    pthread_cond_signal(&rb->not_full);
    pthread_mutex_unlock(&rb->lock);

    return 0;
}

const char *ring_buffer_strerror(int err) {
    switch (err) {
        case 0:                        return "success";
        case RB_ERR_INVALID_ARG:       return "invalid argument (NULL pointer, or zero capacity)";
        case RB_ERR_ALLOC_FAILED:      return "malloc() for the sample buffer failed";
        case RB_ERR_NOT_POWER_OF_TWO:  return "capacity was not a power of two";
        case RB_ERR_MUTEX_INIT_FAILED: return "pthread_mutex_init() failed";
        case RB_ERR_COND_INIT_FAILED:  return "pthread_cond_init() failed";
        case RB_ERR_NOT_INITIALIZED:   return "ring buffer is not initialized, or has already been destroyed";
        case RB_ERR_EMPTY:             return "ring buffer is empty";
        default:                       return "unknown error";
    }
}
