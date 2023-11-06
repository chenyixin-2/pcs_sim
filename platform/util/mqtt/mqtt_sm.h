#ifndef __MQTT_SM_H__
#define __MQTT_SM_H__

enum mqtt_err_t{
    MQTTERR_NONE = 0,
    
    // offline
    MQTTERR_OFFLINE_PWRUP,
};

int mqtt_sm_init();
void mqtt_sm( void );

#endif