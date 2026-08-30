#include "ring_buffer.h"
#include <stdlib.h>

int ring_buffer_init(ring_buffer_t *rb, size_t capacity) {
    if (rb == NULL || capacity == 0) {
        return RB_ERR_INVALID_ARG;
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

    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

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
    rb->count = 0;

    pthread_mutex_unlock(&rb->lock);

    pthread_mutex_destroy(&rb->lock);
}


// Push writes at the tail; when full it also advances the head to drop the oldest.
int ring_buffer_push(ring_buffer_t *rb, telemetry_sample_t sample) {
    if (rb == NULL) {
        return RB_ERR_INVALID_ARG;
    }

    pthread_mutex_lock(&rb->lock);

    if (rb->buffer == NULL) {
        pthread_mutex_unlock(&rb->lock);
        return RB_ERR_NOT_INITIALIZED;
    }

    if (rb->count == rb->capacity) {
        // Buffer is full: write at tail (== head when full), then advance
        // head to discard the oldest sample
        rb->buffer[rb->tail] = sample;
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->head = (rb->head + 1) % rb->capacity; // Discard oldest
    } else {
        // Normal enqueue at the tail
        rb->buffer[rb->tail] = sample;
        rb->tail = (rb->tail + 1) % rb->capacity;
        rb->count++;
    }

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

    if (rb->count == 0) {
        pthread_mutex_unlock(&rb->lock);
        return RB_ERR_EMPTY;
    }
    
    // Pop the oldest sample from the head
    *sample = rb->buffer[rb->head];
    rb->head = (rb->head + 1) % rb->capacity;
    rb->count--;

    pthread_mutex_unlock(&rb->lock);

    return 0;
}


const char *ring_buffer_strerror(int err) {
    switch (err) {
        case 0:                        return "success";
        case RB_ERR_INVALID_ARG:       return "invalid argument (NULL pointer, or zero capacity)";
        case RB_ERR_ALLOC_FAILED:      return "malloc() for the sample buffer failed";
        case RB_ERR_MUTEX_INIT_FAILED: return "pthread_mutex_init() failed";
        case RB_ERR_NOT_INITIALIZED:   return "ring buffer is not initialized, or has already been destroyed";
        case RB_ERR_EMPTY:             return "ring buffer is empty";
        default:                       return "unknown error";
    }
}
