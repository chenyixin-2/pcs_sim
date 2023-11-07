#include "plt.h"
#include "mqtt_sm.h"
static struct state_t mqtt_states[] = {
    {SMST_OFFLINE, "offline"},
    {SMST_READY, "ready"},
};

static struct err_t mqtt_errs[] = {
    {MQTTERR_NONE, "none"},

    // offline
    {MQTTERR_OFFLINE_PWRUP, "pwrup"},
};

int mqtt_sm_init()
{
    struct statemachine_t *sm = &MDL.mqtt.sm;

    sm_reset_timing(sm, 100, 25);
    sm->states = mqtt_states;
    sm->state_nbr = sizeof(mqtt_states) / sizeof(struct state_t);
    sm->errs = mqtt_errs;
    sm->err_nbr = sizeof(mqtt_errs) / sizeof(struct err_t);
    sm_set_state(sm, SMST_OFFLINE, MQTTERR_OFFLINE_PWRUP);

    return 0;
}

static void mqtt_sm_offline()
{
    struct mqtt_t *dev = &MDL.mqtt;
    struct statemachine_t *sm = &MDL.mqtt.sm;

    log_dbg("mqtt sm offline, state:%s, step:%d", sm_get_szstate(sm), sm_get_step(sm));
    if (sm_get_step(sm) == 0)
    { // entry
        mqtt_reset_cmd();
        sm_set_step(sm, 10);
    }
    else if (sm_get_step(sm) == 10)
    { // wait cmd
        if (mqtt_get_cmd() == CMD_SM_READY)
        { // ready cmd
            log_dbg("%s, state:%s, step:%d, get ready cmd, try to connect", __func__, sm_get_szstate(sm), sm_get_step(sm));
            mqtt_reset_cmd();
            if (mqtt_connect() < 0)
            {
                if (dev->cli != NULL)
                {
                    MQTTClient_destroy(&dev->cli);
                }
                log_dbg("%s, state:%s, step:%d, connect fail, re-connet 10 seconds later", __func__, sm_get_szstate(sm), sm_get_step(sm));
                sm_set_count(sm, 0);
            }
            else
            {
                dev->connlost = 0;
                log_dbg("%s, state:%s, step:%d, connect ok, goto ready", __func__, sm_get_szstate(sm), sm_get_step(sm));
                sm_set_state(sm, SMST_READY, MQTTERR_NONE);
            }
        }
        else
        {
            sm_inc_count(sm);
            log_dbg("%s, state:%s, step:%d, wait ready cmd, count:%d", __func__, sm_get_szstate(sm), sm_get_step(sm), sm_get_count(sm));
            if (sm_get_count(sm) >= 10)
            { /* 10 seconds */                
                sm_set_count(sm, 0);
                if (dev->enable)
                {
                    log_dbg("%s, state:%s, step:%d, 10 seconds passed, try to connect", __func__, sm_get_szstate(sm), sm_get_step(sm));
                    if (mqtt_connect() < 0)
                    {
                        if (dev->cli != NULL)
                        {
                            MQTTClient_destroy(&dev->cli);
                        }
                        log_dbg("%s, state:%s, step:%d, connect fail, re-connet 10 seconds later", __func__, sm_get_szstate(sm), sm_get_step(sm));
                        sm_set_count(sm, 0);
                    }
                    else
                    {
                        dev->connlost = 0;
                        log_dbg("%s, state:%s, step:%d, connect ok, goto stdby", __func__, sm_get_szstate(sm), sm_get_step(sm));
                        sm_set_state(sm, SMST_READY, MQTTERR_NONE);
                    }
                }
                else
                {
                    ; /* nothing to do */
                }
            }
        }
    }
}

static void mqtt_sm_ready()
{
    struct mqtt_t *dev = &MDL.mqtt;
    struct statemachine_t *sm = &MDL.mqtt.sm;
    int rc = 0;
    mqtt_ringbuffer_element_t e;
    char *topicName = NULL;
    int topicLen;
    MQTTClient_message *message = NULL;
    char *pdst = NULL;
    char *psrc = NULL;
    int i;

    if (sm_get_step(sm) == 0)
    { // entry
        mqtt_reset_cmd();
        sm_set_step(sm, 10);
    }
    else if (sm_get_step(sm) == 10)
    { // wait and chk
        if (mqtt_get_cmd() == CMD_SM_OFFLINE)
        { // offline cmd
            log_dbg("%s, state:%s, step:%d, get offline cmd, goto offline",
                    __func__, sm_get_szstate(sm), sm_get_step(sm));
            mqtt_reset_cmd();
            if (dev->cli != NULL)
            {
                MQTTClient_disconnect(dev->cli, 0);
                MQTTClient_destroy(&dev->cli);
            }
            sm_set_state(sm, SMST_OFFLINE, MQTTERR_NONE);
        }
        else if (dev->connlost)
        {
            log_dbg("%s, state:%s, step:%d, connection lost detected, goto offline", __func__, sm_get_szstate(sm), sm_get_step(sm));
            if (dev->cli != NULL)
            {
                MQTTClient_destroy(&dev->cli);
            }
            sm_set_state(sm, SMST_OFFLINE, MQTTERR_NONE);
        }
        else
        {
            // rc = MQTTClient_receive(dev->cli, &topicName, &topicLen, &message, 30);
            // if (message){
            //     log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName,topicLen, message->payloadlen);
            //     pdst = e.szpayload;
            //     psrc = message->payload;
            //     for( i = 0; i < message->payloadlen; i++ ){
            //         *pdst++ = *psrc++;
            //     }
            //     *pdst = 0;
            //     log_dbg("%s, payload:%s", __func__, e.szpayload);
            //     MQTTClient_freeMessage(&message);
            //     MQTTClient_free(topicName);
            //     mqtt_lock_rxbuf();
            //     mqtt_queue_rxbuf(e);
            //     mqtt_unlock_rxbuf();
            // }
            // if (rc != 0){
            //     dev->connlost = 1;
            //     log_dbg("%s, MQTTClient_receive fail:%d, set connlost=1", __func__, rc);
            // }

            mqtt_lock_txbuf();
            rc = mqtt_peek_txbuf(&e, 0);
            mqtt_unlock_txbuf();
            if (rc == 1)
            {
                if (mqtt_pub(e.sztopic, e.szpayload) == 0)
                {
                    mqtt_lock_txbuf();
                    mqtt_dequeue_txbuf(&e);
                    mqtt_unlock_txbuf();
                }
                else
                {
                    log_dbg("failed to pub: %s : %s", e.sztopic, e.szpayload);
                }
            }
        }
    }
}

void mqtt_sm(void)
{
    struct mqtt_t *dev = &MDL.mqtt;
    struct statemachine_t *sm = &dev->sm;

    sm_cal_timing(sm);

    dev->txbuf_usage = (double)mqtt_get_txbuf_used() / (double)mqtt_get_txbuf_size() * 100;
    dev->rxbuf_usage = (double)mqtt_get_rxbuf_used() / (double)mqtt_get_rxbuf_size() * 100;

    log_dbg("%s, mqtt state : %d", __func__, mqtt_get_state());

    switch (mqtt_get_state())
    {
    case SMST_OFFLINE:
        mqtt_sm_offline();
        break;
    case SMST_READY:
        mqtt_sm_ready();
        break;

    default:
        log_dbg("%s, never reach here", __func__);
        break;
    }
}