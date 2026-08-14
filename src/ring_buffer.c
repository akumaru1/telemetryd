#include "ring_buffer.h"
#include <stdlib.h>

int ring_buffer_init(ring_buffer_t *rb, size_t capacity) {
    if (rb == NULL || capacity == 0) {
        return -1;
    }
    
    rb->buffer = malloc(capacity * sizeof(telemetry_sample_t));
    if (rb->buffer == NULL) {
        return -2;
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
    
    if (rb->buffer != NULL) {
        free(rb->buffer);
        rb->buffer = NULL;
    }
    
    rb->capacity = 0;
    rb->head = 0;
    rb->tail = 0;
    rb->size = 0;
}
