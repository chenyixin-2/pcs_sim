#ifndef __MQTT_RINGBUFFER_H__
#define __MQTT_RINGBUFFER_H__

#include <pthread.h>

/** * The size of a ring buffer.
    * Due to the design only <tt> RING_BUFFER_SIZE-1 </tt> items
    * can be contained in the buffer.
    * The buffer size must be a power of two.
*/
#define MQTT_RING_BUFFER_SIZE 0x100
#if (MQTT_RING_BUFFER_SIZE & (MQTT_RING_BUFFER_SIZE - 1)) != 0
#error "RING_BUFFER_SIZE must be a power of two"
#endif

/**
    * Used as a modulo operator
    * as <tt> a % b = (a & (b ? 1)) </tt>
    * where \c a is a positive index in the buffer and
    * \c b is the (power of two) size of the buffer.
*/
#define MQTT_RING_BUFFER_MASK (MQTT_RING_BUFFER_SIZE-1)

/** * The type which is used to hold the size
    * and the indicies of the buffer.
    * Must be able to fit \c RING_BUFFER_SIZE .
*/
//typedef uint8_t ring_buffer_size_t;
typedef int mqtt_ringbuffer_size_t;

typedef struct tag_mqtt_ringbuffer_element{
    int idx;
    int cmd;
    int val;
    char sztopic[128];
    char szpayload[8000];
}mqtt_ringbuffer_element_t;

/**
  * Simplifies the use of <tt>struct ring_buffer_t</tt>.
  */
typedef struct mqtt_ringbuffer_t mqtt_ringbuffer_t;

/**
    * Structure which holds a ring buffer.
    * The buffer contains a buffer array
    * as well as metadata for the ring buffer.
    */
struct mqtt_ringbuffer_t {
    /** Buffer memory. */
    mqtt_ringbuffer_element_t buffer[MQTT_RING_BUFFER_SIZE];
    /** Index of tail. */
    mqtt_ringbuffer_size_t tail_index;
    /** Index of head. */
    mqtt_ringbuffer_size_t head_index;
};

void mqtt_ringbuffer_init(mqtt_ringbuffer_t *buffer);
void mqtt_ringbuffer_queue(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t data);
void mqtt_ringbuffer_queue_arr(mqtt_ringbuffer_t *buffer, const mqtt_ringbuffer_element_t*data, mqtt_ringbuffer_size_t size);
mqtt_ringbuffer_size_t mqtt_ringbuffer_dequeue(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t* data);
mqtt_ringbuffer_size_t mqtt_ringbuffer_dequeue_arr(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t* data, mqtt_ringbuffer_size_t len);
mqtt_ringbuffer_size_t mqtt_ringbuffer_peek(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t* data, mqtt_ringbuffer_size_t index);
mqtt_ringbuffer_size_t mqtt_ringbuffer_num_items(mqtt_ringbuffer_t *buffer);
mqtt_ringbuffer_size_t mqtt_ringbuffer_size(mqtt_ringbuffer_t *buffer);
#endif