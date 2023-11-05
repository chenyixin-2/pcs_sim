#ifndef __TBMQTT_CACHE_H__
#define __TBMQTT_CACHE_H__

#include "tbmqtt_ringbuffer.h"


int tbmqtt_cache_open(char* dir, char* table_name, void** handle);
int tbmqtt_cache_read_one_payload(void* handle, char* table_name, tbmqtt_ringbuffer_element_t* e, int* idx);
int tbmqtt_cache_write_one_payload(void* handle, char* table_name, tbmqtt_ringbuffer_element_t e);
int tbmqtt_cache_delete_one_payload(void* handle, char* table_name,int rowid);
int tbmqtt_cache_get_payload_nb(void* handle, char* table_name, int* nb);
int tbmqtt_cache_get_memory_size(void* handle, long* size);
void tbmqtt_cache_close(void* handle);
int tbmqtt_cache_free_memory(void* handle);

#endif