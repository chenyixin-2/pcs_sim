#include "test_ringbuffer.h"

test_ring_buffer_t testrb;
test_ring_buffer_t* ptestrb = &testrb;
/** * Initializes the ring buffer pointed to by <em>buffer</em>.
    * This function can also be used to empty/reset the buffer.
    * @param buffer The ring buffer to initialize.
    */
void test_ring_buffer_init(test_ring_buffer_t *buffer)
{
    buffer->tail_index = 0;
    buffer->head_index = 0;
}

/** * Adds a byte to a ring buffer.
    * @param buffer The buffer in which the data should be placed.
    * @param data The byte to place.
    */
void test_ring_buffer_queue(test_ring_buffer_t *buffer, test_ring_buffer_element_t data)
{
    /* Is buffer full? */
    if(test_ring_buffer_is_full(buffer)) {
        /* Is going to overwrite the oldest byte */
        /* Increase tail index */
        buffer->tail_index = ((buffer->tail_index + 1) & TEST_RING_BUFFER_MASK);
    }

    /* Place data in buffer */
    buffer->buffer[buffer->head_index] = data;
    buffer->head_index = ((buffer->head_index + 1) & TEST_RING_BUFFER_MASK);
}

/** * Adds an array of bytes to a ring buffer.
    * @param buffer The buffer in which the data should be placed.
    * @param data A pointer to the array of bytes to place in the queue.
    * @param size The size of the array.
    */
void test_ring_buffer_queue_arr(test_ring_buffer_t *buffer, const test_ring_buffer_element_t*data, test_ring_buffer_size_t size)
{
    /* Add bytes; one by one */
    test_ring_buffer_size_t i;
    for(i = 0; i < size; i++) {
        test_ring_buffer_queue(buffer, data[i]);
    }
}

/** * Returns the oldest byte in a ring buffer.
    * @param buffer The buffer from which the data should be returned.
    * @param data A pointer to the location at which the data should be placed.
    * @return 1 if data was returned; 0 otherwise.
    */
test_ring_buffer_size_t test_ring_buffer_dequeue(test_ring_buffer_t *buffer, test_ring_buffer_element_t* data)
{
    if(test_ring_buffer_is_empty(buffer)) {
        /* No items */
        return 0;
    }
    *data = buffer->buffer[buffer->tail_index];
    buffer->tail_index = ((buffer->tail_index + 1) & TEST_RING_BUFFER_MASK);
    return 1;
}

/** * Returns the <em>len</em> oldest bytes in a ring buffer.
    * @param buffer The buffer from which the data should be returned.
    * @param data A pointer to the array at which the data should be placed.
    * @param len The maximum number of bytes to return.
    * @return The number of bytes returned.
    */
test_ring_buffer_size_t test_ring_buffer_dequeue_arr(test_ring_buffer_t *buffer, test_ring_buffer_element_t* data, test_ring_buffer_size_t len)
{
    if(test_ring_buffer_is_empty(buffer)) {
        /* No items */
        return 0;
    }
    test_ring_buffer_element_t* data_ptr = data;
    test_ring_buffer_size_t cnt = 0;
    while((cnt < len) && test_ring_buffer_dequeue(buffer, data_ptr)) {
        cnt++;
        data_ptr++;
    }
    return cnt;
}

/** * Peeks a ring buffer, i.e. returns an element without removing it.
    * @param buffer The buffer from which the data should be returned.
    * @param data A pointer to the location at which the data should be placed.
    * @param index The index to peek. * @return 1 if data was returned; 0 otherwise.
    */
test_ring_buffer_size_t test_ring_buffer_peek(test_ring_buffer_t *buffer, test_ring_buffer_element_t* data, test_ring_buffer_size_t index)
{
    if(index >= test_ring_buffer_num_items(buffer)) {
        /* No items at index */
        return 0;
    }
    /* Add index to pointer */
    test_ring_buffer_size_t data_index = ((buffer->tail_index + index) & TEST_RING_BUFFER_MASK);
    *data = buffer->buffer[data_index];
    return 1;
}

/** * Returns the number of items in a ring buffer.
    * @param buffer The buffer for which the number of items should be returned.
    * @return The number of items in the ring buffer.
    */
test_ring_buffer_size_t test_ring_buffer_num_items(test_ring_buffer_t *buffer)
{
    return ((buffer->head_index - buffer->tail_index) & TEST_RING_BUFFER_MASK);
}

test_ring_buffer_size_t test_ring_buffer_size(test_ring_buffer_t *buffer)
{
    return TEST_RING_BUFFER_SIZE;
}

/** * Returns whether a ring buffer is empty.
    * @param buffer The buffer for which it should be returned whether it is empty.
    * @return 1 if empty; 0 otherwise.
    */
inline test_ring_buffer_size_t test_ring_buffer_is_empty(test_ring_buffer_t *buffer)
{
    return (buffer->head_index == buffer->tail_index);
}

/** * Returns whether a ring buffer is full.
    * @param buffer The buffer for which it should be returned whether it is full.
    * @return 1 if full; 0 otherwise.
    */
inline test_ring_buffer_size_t test_ring_buffer_is_full(test_ring_buffer_t *buffer)
{
    return ((buffer->head_index - buffer->tail_index) & TEST_RING_BUFFER_MASK) == TEST_RING_BUFFER_MASK;
}