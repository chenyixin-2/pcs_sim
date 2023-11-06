#ifndef __MQTT_CACHE_H__
#define __MQTT_CACHE_H__

#include "mqtt_ringbuffer.h"


int mqtt_cache_open(const char* dir, const char* table_name, void** handle);
int mqtt_cache_read_one_payload(void *handle, const char *table_name, mqtt_ringbuffer_element_t *e, int *idx);
int mqtt_cache_write_one_payload(void *handle, const char *table_name, mqtt_ringbuffer_element_t e);
int mqtt_cache_delete_one_payload(void* handle, const char* table_name,int rowid);
int mqtt_cache_get_payload_nb(void* handle, const char* table_name, int* nb);
int mqtt_cache_get_memory_size(void* handle, long* size);
void mqtt_cache_close(void* handle);
int mqtt_cache_free_memory(void* handle);

#endif