#ifndef __TBMQTT_RINGBUFFER_H__
#define __TBMQTT_RINGBUFFER_H__

#include <pthread.h>

#define BUF_PAYLOAD_MAX_LEN     (8000)
#define BUF_TOPIC_MAX_LEN       (128)

/** * The size of a ring buffer.
 * Due to the design only <tt> RING_BUFFER_SIZE-1 </tt> items
 * can be contained in the buffer.
 * The buffer size must be a power of two.
 */
#define TBMQTT_RINGBUFFER_SIZE 0x200
#if (TBMQTT_RINGBUFFER_SIZE & (TBMQTT_RINGBUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of two"
#endif

/**
 * Used as a modulo operator
 * as <tt> a % b = (a & (b ? 1)) </tt>
 * where \c a is a positive index in the buffer and
 * \c b is the (power of two) size of the buffer.
 */
#define TBMQTT_RINGBUFFER_MASK (TBMQTT_RINGBUFFER_SIZE - 1)

/** * The type which is used to hold the size
 * and the indicies of the buffer.
 * Must be able to fit \c RING_BUFFER_SIZE .
 */
// typedef uint8_t ring_buffer_size_t;
typedef int tbmqtt_ringbuffer_size_t;

typedef struct tag_tbmqtt_ringbuffer_element
{
    int idx;
    int cmd;
    int val;
    char sztopic[BUF_TOPIC_MAX_LEN];
    char szpayload[BUF_PAYLOAD_MAX_LEN];
} tbmqtt_ringbuffer_element_t;

/**
 * Simplifies the use of <tt>struct ring_buffer_t</tt>.
 */
typedef struct tbmqtt_ringbuffer_t tbmqtt_ringbuffer_t;

/**
 * Structure which holds a ring buffer.
 * The buffer contains a buffer array
 * as well as metadata for the ring buffer.
 */
struct tbmqtt_ringbuffer_t
{
    /** Buffer memory. */
    tbmqtt_ringbuffer_element_t buffer[TBMQTT_RINGBUFFER_SIZE];
    /** Index of tail. */
    tbmqtt_ringbuffer_size_t tail_index;
    /** Index of head. */
    tbmqtt_ringbuffer_size_t head_index;
};

void tbmqtt_ringbuffer_init(tbmqtt_ringbuffer_t *buffer);
void tbmqtt_ringbuffer_queue(tbmqtt_ringbuffer_t *buffer, tbmqtt_ringbuffer_element_t data);
void tbmqtt_ringbuffer_queue_arr(tbmqtt_ringbuffer_t *buffer, const tbmqtt_ringbuffer_element_t *data, tbmqtt_ringbuffer_size_t size);
tbmqtt_ringbuffer_size_t tbmqtt_ringbuffer_dequeue(tbmqtt_ringbuffer_t *buffer, tbmqtt_ringbuffer_element_t *data);
tbmqtt_ringbuffer_size_t tbmqtt_ringbuffer_dequeue_arr(tbmqtt_ringbuffer_t *buffer, tbmqtt_ringbuffer_element_t *data, tbmqtt_ringbuffer_size_t len);
tbmqtt_ringbuffer_size_t tbmqtt_ringbuffer_peek(tbmqtt_ringbuffer_t *buffer, tbmqtt_ringbuffer_element_t *data, tbmqtt_ringbuffer_size_t index);
tbmqtt_ringbuffer_size_t tbmqtt_ringbuffer_num_items(tbmqtt_ringbuffer_t *buffer);
tbmqtt_ringbuffer_size_t tbmqtt_ringbuffer_size(tbmqtt_ringbuffer_t *buffer);
tbmqtt_ringbuffer_size_t tbmqtt_ringbuffer_is_empty(tbmqtt_ringbuffer_t *buffer);
tbmqtt_ringbuffer_size_t tbmqtt_ringbuffer_is_full(tbmqtt_ringbuffer_t *buffer);
#endif