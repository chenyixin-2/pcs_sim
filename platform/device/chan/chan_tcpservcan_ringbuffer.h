#ifndef __CHAN_TCPSERVCAN_RINGBUFFER_H__
#define __CHAN_TCPSERVCAN_RINGBUFFER_H__

#include <pthread.h>
#include "can_frame.h"

/** * The size of a ring buffer.
    * Due to the design only <tt> RING_BUFFER_SIZE-1 </tt> items
    * can be contained in the buffer.
    * The buffer size must be a power of two.
*/
#define CHAN_TCPSERVCAN_RINGBUFFER_SIZE 0x80
#if (CHAN_TCPSERVCAN_RINGBUFFER_SIZE & (CHAN_TCPSERVCAN_RINGBUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of two"
#endif

/**
    * Used as a modulo operator
    * as <tt> a % b = (a & (b ? 1)) </tt>
    * where \c a is a positive index in the buffer and
    * \c b is the (power of two) size of the buffer.
*/
#define CHAN_TCPSERVCAN_RINGBUFFER_MASK (CHAN_TCPSERVCAN_RINGBUFFER_SIZE-1)

/** * The type which is used to hold the size
    * and the indicies of the buffer.
    * Must be able to fit \c RING_BUFFER_SIZE .
*/
//typedef uint8_t ring_buffer_size_t;
typedef int chan_tcpservcan_ringbuffer_size_t;

typedef struct tag_chan_tcpservcan_ringbuffer_element{
    struct can_frame frame;
}chan_tcpservcan_ringbuffer_element_t;

/**
  * Simplifies the use of <tt>struct ring_buffer_t</tt>.
  */
typedef struct chan_tcpservcan_ringbuffer_t chan_tcpservcan_ringbuffer_t;

/**
    * Structure which holds a ring buffer.
    * The buffer contains a buffer array
    * as well as metadata for the ring buffer.
    */
struct chan_tcpservcan_ringbuffer_t {
    /** Buffer memory. */
    chan_tcpservcan_ringbuffer_element_t buffer[CHAN_TCPSERVCAN_RINGBUFFER_SIZE];
    /** Index of tail. */
    chan_tcpservcan_ringbuffer_size_t tail_index;
    /** Index of head. */
    chan_tcpservcan_ringbuffer_size_t head_index;
};

chan_tcpservcan_ringbuffer_size_t chan_tcpservcan_ringbuffer_is_empty(chan_tcpservcan_ringbuffer_t *buffer);

chan_tcpservcan_ringbuffer_size_t chan_tcpservcan_ringbuffer_is_full(chan_tcpservcan_ringbuffer_t *buffer);

void chan_tcpservcan_ringbuffer_init(chan_tcpservcan_ringbuffer_t *buffer);
void chan_tcpservcan_ringbuffer_queue(chan_tcpservcan_ringbuffer_t *buffer, chan_tcpservcan_ringbuffer_element_t data);
void chan_tcpservcan_ringbuffer_queue_arr(chan_tcpservcan_ringbuffer_t *buffer, const chan_tcpservcan_ringbuffer_element_t*data, chan_tcpservcan_ringbuffer_size_t size);
chan_tcpservcan_ringbuffer_size_t chan_tcpservcan_ringbuffer_dequeue(chan_tcpservcan_ringbuffer_t *buffer, chan_tcpservcan_ringbuffer_element_t* data);
chan_tcpservcan_ringbuffer_size_t chan_tcpservcan_ringbuffer_dequeue_arr(chan_tcpservcan_ringbuffer_t *buffer, chan_tcpservcan_ringbuffer_element_t* data, chan_tcpservcan_ringbuffer_size_t len);
chan_tcpservcan_ringbuffer_size_t chan_tcpservcan_ringbuffer_peek(chan_tcpservcan_ringbuffer_t *buffer, chan_tcpservcan_ringbuffer_element_t* data, chan_tcpservcan_ringbuffer_size_t index);
chan_tcpservcan_ringbuffer_size_t chan_tcpservcan_ringbuffer_num_items(chan_tcpservcan_ringbuffer_t *buffer);

#endif