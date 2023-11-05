#ifndef __MQTT_H__
#define __MQTT_H__

int mqtt_init(void);
int mqtt_set_state( int state, int errcode);
int mqtt_send_cmd( int cmd);
void mqtt_lock_txbuf();
void mqtt_unlock_txbuf();
int mqtt_get_rb_used( void );
int mqtt_get_rb_size( void );
void mqtt_init_txbuf();
void mqtt_lock_txbuf();
void mqtt_unlock_txbuf();
void mqtt_queue_txbuf(mqtt_ringbuffer_element_t data);
int mqtt_dequeue_txbuf(mqtt_ringbuffer_element_t* data);
int mqtt_peek_txbuf(mqtt_ringbuffer_element_t* data, mqtt_ringbuffer_size_t index);
int mqtt_get_txbuf_used( void );
int mqtt_get_txbuf_size( void );

void mqtt_init_rxbuf();
void mqtt_lock_rxbuf();
void mqtt_unlock_rxbuf();
void mqtt_queue_rxbuf(mqtt_ringbuffer_element_t data);
int mqtt_dequeue_rxbuf(mqtt_ringbuffer_element_t* data);
int mqtt_peek_rxbuf(mqtt_ringbuffer_element_t* data, mqtt_ringbuffer_size_t index);
int mqtt_get_rxbuf_used( void );
int mqtt_get_rxbuf_size( void );
#endif