#ifndef __CHAN_RINGBUFFER_H__
#define __CHAN_RINGBUFFER_H__

#include <pthread.h>

/** * The size of a ring buffer.
    * Due to the design only <tt> RING_BUFFER_SIZE-1 </tt> items
    * can be contained in the buffer.
    * The buffer size must be a power of two.
*/
#define CHAN_RING_BUFFER_SIZE 0x80
#if (CHAN_RING_BUFFER_SIZE & (CHAN_RING_BUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of two"
#endif

/**
    * Used as a modulo operator
    * as <tt> a % b = (a & (b ? 1)) </tt>
    * where \c a is a positive index in the buffer and
    * \c b is the (power of two) size of the buffer.
*/
#define CHAN_RING_BUFFER_MASK (CHAN_RING_BUFFER_SIZE-1)

/** * The type which is used to hold the size
    * and the indicies of the buffer.
    * Must be able to fit \c RING_BUFFER_SIZE .
*/
//typedef uint8_t ring_buffer_size_t;
typedef int chan_ring_buffer_size_t;

typedef struct tag_chan_ring_buffer_element{
    char c;
}chan_ring_buffer_element_t;

/**
  * Simplifies the use of <tt>struct ring_buffer_t</tt>.
  */
typedef struct chan_ring_buffer_t chan_ring_buffer_t;

/**
    * Structure which holds a ring buffer.
    * The buffer contains a buffer array
    * as well as metadata for the ring buffer.
    */
struct chan_ring_buffer_t {
    /** Buffer memory. */
    chan_ring_buffer_element_t buffer[CHAN_RING_BUFFER_SIZE];
    /** Index of tail. */
    chan_ring_buffer_size_t tail_index;
    /** Index of head. */
    chan_ring_buffer_size_t head_index;
};

void chan_ring_buffer_init(chan_ring_buffer_t *buffer);
void chan_ring_buffer_queue(chan_ring_buffer_t *buffer, chan_ring_buffer_element_t data);
void chan_ring_buffer_queue_arr(chan_ring_buffer_t *buffer, const chan_ring_buffer_element_t*data, chan_ring_buffer_size_t size);
chan_ring_buffer_size_t chan_ring_buffer_dequeue(chan_ring_buffer_t *buffer, chan_ring_buffer_element_t* data);
chan_ring_buffer_size_t chan_ring_buffer_dequeue_arr(chan_ring_buffer_t *buffer, chan_ring_buffer_element_t* data, chan_ring_buffer_size_t len);
chan_ring_buffer_size_t chan_ring_buffer_peek(chan_ring_buffer_t *buffer, chan_ring_buffer_element_t* data, chan_ring_buffer_size_t index);
chan_ring_buffer_size_t chan_ring_buffer_num_items(chan_ring_buffer_t *buffer);

#endif