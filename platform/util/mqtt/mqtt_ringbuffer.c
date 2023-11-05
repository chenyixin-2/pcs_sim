#include "mqtt_ringbuffer.h"

/** * Initializes the ring buffer pointed to by <em>buffer</em>.
    * This function can also be used to empty/reset the buffer.
    * @param buffer The ring buffer to initialize.
    */
void mqtt_ringbuffer_init(mqtt_ringbuffer_t *buffer)
{
    buffer->tail_index = 0;
    buffer->head_index = 0;
}

/** * Adds a byte to a ring buffer.
    * @param buffer The buffer in which the data should be placed.
    * @param data The byte to place.
    */
void mqtt_ringbuffer_queue(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t data)
{
    /* Is buffer full? */
    if(mqtt_ringbuffer_is_full(buffer)) {
        /* Is going to overwrite the oldest byte */
        /* Increase tail index */
        buffer->tail_index = ((buffer->tail_index + 1) & MQTT_RING_BUFFER_MASK);
    }

    /* Place data in buffer */
    buffer->buffer[buffer->head_index] = data;
    buffer->head_index = ((buffer->head_index + 1) & MQTT_RING_BUFFER_MASK);
}

/** * Adds an array of bytes to a ring buffer.
    * @param buffer The buffer in which the data should be placed.
    * @param data A pointer to the array of bytes to place in the queue.
    * @param size The size of the array.
    */
void mqtt_ringbuffer_queue_arr(mqtt_ringbuffer_t *buffer, const mqtt_ringbuffer_element_t*data, mqtt_ringbuffer_size_t size)
{
    /* Add bytes; one by one */
    mqtt_ringbuffer_size_t i;
    for(i = 0; i < size; i++) {
        mqtt_ringbuffer_queue(buffer, data[i]);
    }
}

/** * Returns the oldest byte in a ring buffer.
    * @param buffer The buffer from which the data should be returned.
    * @param data A pointer to the location at which the data should be placed.
    * @return 1 if data was returned; 0 otherwise.
    */
mqtt_ringbuffer_size_t mqtt_ringbuffer_dequeue(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t* data)
{
    if(mqtt_ringbuffer_is_empty(buffer)) {
        /* No items */
        return 0;
    }
    *data = buffer->buffer[buffer->tail_index];
    buffer->tail_index = ((buffer->tail_index + 1) & MQTT_RING_BUFFER_MASK);
    return 1;
}

/** * Returns the <em>len</em> oldest bytes in a ring buffer.
    * @param buffer The buffer from which the data should be returned.
    * @param data A pointer to the array at which the data should be placed.
    * @param len The maximum number of bytes to return.
    * @return The number of bytes returned.
    */
mqtt_ringbuffer_size_t mqtt_ringbuffer_dequeue_arr(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t* data, mqtt_ringbuffer_size_t len)
{
    if(mqtt_ringbuffer_is_empty(buffer)) {
        /* No items */
        return 0;
    }
    mqtt_ringbuffer_element_t* data_ptr = data;
    mqtt_ringbuffer_size_t cnt = 0;
    while((cnt < len) && mqtt_ringbuffer_dequeue(buffer, data_ptr)) {
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
mqtt_ringbuffer_size_t mqtt_ringbuffer_peek(mqtt_ringbuffer_t *buffer, mqtt_ringbuffer_element_t* data, mqtt_ringbuffer_size_t index)
{
    if(index >= mqtt_ringbuffer_num_items(buffer)) {
        /* No items at index */
        return 0;
    }
    /* Add index to pointer */
    mqtt_ringbuffer_size_t data_index = ((buffer->tail_index + index) & MQTT_RING_BUFFER_MASK);
    *data = buffer->buffer[data_index];
    return 1;
}

/** * Returns the number of items in a ring buffer.
    * @param buffer The buffer for which the number of items should be returned.
    * @return The number of items in the ring buffer.
    */
mqtt_ringbuffer_size_t mqtt_ringbuffer_num_items(mqtt_ringbuffer_t *buffer)
{
    return ((buffer->head_index - buffer->tail_index) & MQTT_RING_BUFFER_MASK);
}

mqtt_ringbuffer_size_t mqtt_ringbuffer_size(mqtt_ringbuffer_t *buffer)
{
    return MQTT_RING_BUFFER_SIZE;
}

/** * Returns whether a ring buffer is empty.
    * @param buffer The buffer for which it should be returned whether it is empty.
    * @return 1 if empty; 0 otherwise.
    */
inline mqtt_ringbuffer_size_t mqtt_ringbuffer_is_empty(mqtt_ringbuffer_t *buffer)
{
    return (buffer->head_index == buffer->tail_index);
}

/** * Returns whether a ring buffer is full.
    * @param buffer The buffer for which it should be returned whether it is full.
    * @return 1 if full; 0 otherwise.
    */
inline mqtt_ringbuffer_size_t mqtt_ringbuffer_is_full(mqtt_ringbuffer_t *buffer)
{
    return ((buffer->head_index - buffer->tail_index) & MQTT_RING_BUFFER_MASK) == MQTT_RING_BUFFER_MASK;
}