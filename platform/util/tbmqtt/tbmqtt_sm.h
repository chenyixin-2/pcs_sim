#ifndef TBMQTT_SM_H
#define TBMQTT_SM_H

enum tbmqtt_err_t{
    TBMQTTERR_NONE = 0,
    
    // offline
    TBMQTTERR_OFFLINE_PWRUP,
};

int tbmqtt_sm_init();
void tbmqtt_sm( void );

#endif /* TBMQTT_SM_H */
