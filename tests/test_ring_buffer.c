#define _XOPEN_SOURCE 500
#include "ring_buffer.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_PRODUCERS 4
#define PUSHES_PER_PRODUCER 500
#define BUFFER_CAPACITY 256

typedef struct {
    ring_buffer_t *rb;
    int thread_id;
} thread_arg_t;

void *producer_func(void *arg) {
    thread_arg_t *t_arg = (thread_arg_t *)arg;
    for (int i = 0; i < PUSHES_PER_PRODUCER; i++) {
        telemetry_sample_t sample;
        sample.timestamp = i;
        sample.cpu_temp = (double)t_arg->thread_id;
        sample.ram_utilization = (double)i;
        int res = ring_buffer_push(t_arg->rb, sample);
        assert(res == 0);
    }
    return NULL;
}

void *consumer_func(void *arg) {
    ring_buffer_t *rb = (ring_buffer_t *)arg;
    int popped_count = 0;
    telemetry_sample_t sample;
    
    // We try to pop items. Since producers are pushing concurrently,
    // we do non-blocking pops and retry if empty.
    while (popped_count < 100) {
        int res = ring_buffer_pop(rb, &sample);
        if (res == 0) {
            popped_count++;
        } else {
            usleep(100); // Sleep for 100 microseconds if buffer is temporarily empty
        }
    }
    return NULL;
}

int main(void) {
    ring_buffer_t rb;
    
    // Test 1: Invalid capacity (zero)
    int res_invalid_zero = ring_buffer_init(&rb, 0);
    assert(res_invalid_zero != 0);

    // Test 2: Non-power-of-two capacity is valid (no longer restricted)
    ring_buffer_t rb_non_pow2;
    int res_non_pow2 = ring_buffer_init(&rb_non_pow2, 10);
    assert(res_non_pow2 == 0);
    assert(rb_non_pow2.capacity == 10);
    ring_buffer_destroy(&rb_non_pow2);

    // Test 3: Valid capacity initialization
    int res_valid = ring_buffer_init(&rb, 8);
    assert(res_valid == 0);
    assert(rb.buffer != NULL);
    assert(rb.capacity == 8);
    assert(rb.head == 0);
    assert(rb.tail == 0);
    assert(rb.count == 0);

    // Test 4: Push up to capacity and pop
    for (int i = 1; i <= 8; i++) {
        telemetry_sample_t sample = { .timestamp = i, .cpu_temp = (double)i, .ram_utilization = (double)i };
        int push_res = ring_buffer_push(&rb, sample);
        assert(push_res == 0);
        assert(rb.count == (size_t)i);
    }
    assert(rb.count == 8);
    
    // Test 5: Overwrite oldest when pushing beyond capacity
    // Pushing 9th element: should overwrite the 1st element (value 1)
    telemetry_sample_t sample_9 = { .timestamp = 9, .cpu_temp = 9.0, .ram_utilization = 9.0 };
    int push_res = ring_buffer_push(&rb, sample_9);
    assert(push_res == 0);
    assert(rb.count == 8); // Size remains 8
    
    // Popping should yield elements from 2 to 9
    telemetry_sample_t popped;
    int pop_res;
    for (int i = 2; i <= 9; i++) {
        pop_res = ring_buffer_pop(&rb, &popped);
        assert(pop_res == 0);
        assert(popped.timestamp == i);
    }
    
    // Buffer should now be empty
    assert(rb.count == 0);
    pop_res = ring_buffer_pop(&rb, &popped);
    assert(pop_res != 0); // Should fail
    
    ring_buffer_destroy(&rb);
    
    // Test 6: Concurrency test with multiple threads
    int init_res = ring_buffer_init(&rb, BUFFER_CAPACITY);
    assert(init_res == 0);
    
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumer;
    thread_arg_t args[NUM_PRODUCERS];
    
    // Start consumer thread
    pthread_create(&consumer, NULL, consumer_func, &rb);
    
    // Start producer threads
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        args[i].rb = &rb;
        args[i].thread_id = i;
        pthread_create(&producers[i], NULL, producer_func, &args[i]);
    }
    
    // Wait for all producers to finish
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pthread_join(producers[i], NULL);
    }
    
    // Wait for consumer to finish
    pthread_join(consumer, NULL);
    
    // Clean up
    ring_buffer_destroy(&rb);
    
    printf("ring_buffer_t thread-safe operations tests passed successfully.\n");
    return 0;
}
