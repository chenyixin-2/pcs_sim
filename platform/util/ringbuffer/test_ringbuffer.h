#ifndef TEST_RINGBUFFER_H
#define TEST_RINGBUFFER_H

#include <pthread.h>

/** * The size of a ring buffer.
    * Due to the design only <tt> RING_BUFFER_SIZE-1 </tt> items
    * can be contained in the buffer.
    * The buffer size must be a power of two.
*/
#define TEST_RING_BUFFER_SIZE 0x10
#if (TEST_RING_BUFFER_SIZE & (TEST_RING_BUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of two"
#endif

/**
    * Used as a modulo operator
    * as <tt> a % b = (a & (b ? 1)) </tt>
    * where \c a is a positive index in the buffer and
    * \c b is the (power of two) size of the buffer.
*/
#define TEST_RING_BUFFER_MASK (TEST_RING_BUFFER_SIZE-1)

/** * The type which is used to hold the size
    * and the indicies of the buffer.
    * Must be able to fit \c RING_BUFFER_SIZE .
*/
//typedef uint8_t ring_buffer_size_t;
typedef int test_ring_buffer_size_t;

typedef struct tag_test_ring_buffer_element{
    int val;
    char szstring[128];
}test_ring_buffer_element_t;

/**
  * Simplifies the use of <tt>struct ring_buffer_t</tt>.
  */
typedef struct test_ring_buffer_t test_ring_buffer_t;

/**
    * Structure which holds a ring buffer.
    * The buffer contains a buffer array
    * as well as metadata for the ring buffer.
    */
struct test_ring_buffer_t {
    /** Buffer memory. */
    test_ring_buffer_element_t buffer[TEST_RING_BUFFER_SIZE];
    /** Index of tail. */
    test_ring_buffer_size_t tail_index;
    /** Index of head. */
    test_ring_buffer_size_t head_index;
};

void test_ring_buffer_init(test_ring_buffer_t *buffer);
void test_ring_buffer_queue(test_ring_buffer_t *buffer, test_ring_buffer_element_t data);
void test_ring_buffer_queue_arr(test_ring_buffer_t *buffer, const test_ring_buffer_element_t*data, test_ring_buffer_size_t size);
test_ring_buffer_size_t test_ring_buffer_dequeue(test_ring_buffer_t *buffer, test_ring_buffer_element_t* data);
test_ring_buffer_size_t test_ring_buffer_dequeue_arr(test_ring_buffer_t *buffer, test_ring_buffer_element_t* data, test_ring_buffer_size_t len);
test_ring_buffer_size_t test_ring_buffer_peek(test_ring_buffer_t *buffer, test_ring_buffer_element_t* data, test_ring_buffer_size_t index);
test_ring_buffer_size_t test_ring_buffer_num_items(test_ring_buffer_t *buffer);
test_ring_buffer_size_t test_ring_buffer_size(test_ring_buffer_t *buffer);

extern test_ring_buffer_t* ptestrb;
#endif /* TEST_RINGBUFFER_H */
