#include "ring_buffer.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    ring_buffer_t rb;
    
    // Test 1: Invalid parameters for initialization
    int res_null = ring_buffer_init(NULL, 10);
    assert(res_null != 0);
    
    int res_zero = ring_buffer_init(&rb, 0);
    assert(res_zero != 0);
    
    // Test 2: Valid initialization
    int res_valid = ring_buffer_init(&rb, 10);
    assert(res_valid == 0);
    assert(rb.buffer != NULL);
    assert(rb.capacity == 10);
    assert(rb.head == 0);
    assert(rb.tail == 0);
    assert(rb.size == 0);
    
    // Test 3: Safe destruction of a valid buffer
    ring_buffer_destroy(&rb);
    assert(rb.buffer == NULL);
    assert(rb.capacity == 0);
    assert(rb.head == 0);
    assert(rb.tail == 0);
    assert(rb.size == 0);
    
    // Test 4: Safe destruction of NULL
    ring_buffer_destroy(NULL); // Should not crash
    
    printf("ring_buffer_t tests passed successfully.\n");
    return 0;
}
