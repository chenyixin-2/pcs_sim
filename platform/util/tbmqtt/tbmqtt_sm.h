#ifndef __TBMQTT_SM_H__
#define __TBMQTT_SM_H__

enum tbmqtt_err_t{
    TBMQTTERR_NONE = 0,
    
    // offline
    TBMQTTERR_OFFLINE_PWRUP,
};

int tbmqtt_sm_init();
void tbmqtt_sm( void );

#endif