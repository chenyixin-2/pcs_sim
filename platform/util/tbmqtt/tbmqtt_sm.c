#include "plt.h"
#include "tbmqtt_sm.h"

static struct state_t tbmqtt_states[] = {
    {SMST_OFFLINE, "offline"},
    {SMST_READY, "ready"},
};

static struct err_t tbmqtt_errs[] = {
    {TBMQTTERR_NONE, "none"},

    // offline
    {TBMQTTERR_OFFLINE_PWRUP, "pwrup"},
};

int tbmqtt_sm_init()
{
    struct statemachine_t *sm = &tbmqtt.sm;

    sm_reset_timing(sm, 20, 20);
    sm->states = tbmqtt_states;
    sm->state_nbr = sizeof(tbmqtt_states) / sizeof(struct state_t);
    sm->errs = tbmqtt_errs;
    sm->err_nbr = sizeof(tbmqtt_errs) / sizeof(struct err_t);
    sm_set_state(sm, SMST_OFFLINE, TBMQTTERR_OFFLINE_PWRUP);

    return 0;
}

static void tbmqtt_sm_offline()
{
    struct tbmqtt_t *dev = &tbmqtt;
    struct statemachine_t *sm = &dev->sm;

    if (sm_get_step(sm) == 0)
    { // entry
        log_dbg("%s, state:%s, step:%d, entry", __func__, sm_get_szstate(sm), sm_get_step(sm));
        tbmqtt_reset_cmd();
        sm_set_step(sm, 10);
        sm_set_count(sm, 0);
    }
    else if (sm_get_step(sm) == 10)
    { // wait cmd
        if (tbmqtt_get_cmd() == CMD_SM_READY)
        { // ready cmd
            log_dbg("%s, state:%s, step:%d, get ready cmd, try to connect", __func__, sm_get_szstate(sm), sm_get_step(sm));
            tbmqtt_reset_cmd();
            if (tbmqtt_connect() < 0)
            {
                log_dbg("%s, state:%s, step:%d, tbmqtt connect fail, re-connect 10 seconds later", __func__, sm_get_szstate(sm), sm_get_step(sm));
                if (dev->cli != NULL)
                {
                    MQTTClient_destroy(&dev->cli);
                }
                dev->connlost = 1;
                sm_set_count(sm, 0);
            }
            else
            {
                dev->connlost = 0;
                log_dbg("%s, state:%s, step:%d, connect ok, goto ready", __func__, sm_get_szstate(sm), sm_get_step(sm));
                sm_set_state(sm, SMST_READY, TBMQTTERR_NONE);
            }
        }
        else
        {
            sm_inc_count(sm);
            if (sm_get_count(sm) >= 2000)
            { // 10s
                sm_set_count(sm, 0);
                if (dev->enable)
                {
                    log_dbg("%s, state:%s, step:%d, 10 seconds passed, try to connect", __func__, sm_get_szstate(sm), sm_get_step(sm));
                    if (tbmqtt_connect() < 0)
                    {
                        if (dev->cli != NULL)
                        {
                            MQTTClient_destroy(&dev->cli);
                        }
                        dev->connlost = 1;
                        log_dbg("%s, state:%s, step:%d, connect fail, re-connet 10 seconds later", __func__, sm_get_szstate(sm), sm_get_step(sm));
                        sm_set_count(sm, 0);
                    }
                    else
                    {
                        dev->connlost = 0;
                        log_dbg("%s, state:%s, step:%d, connect ok, goto ready", __func__, sm_get_szstate(sm), sm_get_step(sm));
                        sm_set_state(sm, SMST_READY, TBMQTTERR_NONE);
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

static void tbmqtt_sm_ready()
{
    struct tbmqtt_t *dev = &tbmqtt;
    struct statemachine_t *sm = &dev->sm;
    int rc = 0;
    tbmqtt_ringbuffer_element_t e;
    char *topicName = NULL;
    int topicLen;
    MQTTClient_message *message = NULL;
    char *pdst = NULL;
    char *psrc = NULL;
    int i;

    if (sm_get_step(sm) == 0)
    { // entry
        log_dbg("%s, state:%s, step:%d, entry", __func__, sm_get_szstate(sm), sm_get_step(sm));
        tbmqtt_reset_cmd();
        sm_set_step(sm, 10);
    }
    else if (sm_get_step(sm) == 10)
    { // wait and chk
        if (tbmqtt_get_cmd() == CMD_SM_OFFLINE)
        { // offline cmd
            log_dbg("%s, state:%s, step:%d, get offline cmd, goto offline", __func__, sm_get_szstate(sm), sm_get_step(sm));
            tbmqtt_reset_cmd();
            if (dev->cli != NULL)
            {
                MQTTClient_disconnect(dev->cli, 0);
                MQTTClient_destroy(&dev->cli);
            }
            sm_set_state(sm, SMST_OFFLINE, TBMQTTERR_NONE);
        }
        else if (dev->connlost)
        {
            log_dbg("%s, state:%s, step:%d, connection lost detected, goto offline", __func__, sm_get_szstate(sm), sm_get_step(sm));
            if (dev->cli != NULL)
            {
                MQTTClient_destroy(&dev->cli);
            }
            sm_set_state(sm, SMST_OFFLINE, TBMQTTERR_NONE);
        }
        else
        {
            // log_dbg("%s, ready to publish", __func__);
            // rc = MQTTClient_receive(dev->cli, &topicName, &topicLen, &message, 30);
            // if (message){
            //     pdst = e.sztopic;
            //     psrc = topicName;
            //     for( i = 0; i < topicLen; i++ ){
            //         *pdst++ = *psrc++;
            //     }
            //     *pdst = 0;
            //     pdst = e.szpayload;
            //     psrc = message->payload;
            //     for( i = 0; i < message->payloadlen; i++ ){
            //         *pdst++ = *psrc++;
            //     }
            //     *pdst = 0;
            //     if( dev->dbg ){
            //         log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName,topicLen, message->payloadlen);
            //         log_dbg("%s, payload:%s", __func__, e.szpayload);
            //     }
            //     MQTTClient_freeMessage(&message);
            //     MQTTClient_free(topicName);
            //     tbmqtt_lock_rxbuf();
            //     tbmqtt_queue_rxbuf(e);
            //     tbmqtt_unlock_rxbuf();
            // }
            // if (rc != 0){
            //     dev->connlost = 1;
            //     log_dbg("%s, MQTTClient_receive fail:%d, set connlost=1", __func__, rc);
            // }
            tbmqtt_lock_txbuf();
            rc = tbmqtt_peek_txbuf(&e, 0);
            tbmqtt_unlock_txbuf();
            if (rc == 1)
            {
                if (tbmqtt_pub(e.sztopic, e.szpayload) == 0)
                {
                    // if (dev->pub_failed > dev->pub_maxFailcnt)
                    // {
                    //     dev->pub_maxFailcnt = dev->pub_failed;
                    // }
                    // dev->pub_totalFailcnt += dev->pub_failed;
                    // dev->pub_failed = 0;
                    tbmqtt_lock_txbuf();
                    tbmqtt_dequeue_txbuf(&e);
                    tbmqtt_unlock_txbuf();
                }
                else
                {
                    log_dbg("%s, publish fail, topic:%s, payload:%s", __func__, e.sztopic, e.szpayload);
                    // dev->pub_failed++;
                    // if (dev->pub_failed >= 10)
                    // {
                    //     dev->connlost = 1;
                    // }
                }
            }
        }
    }
}

static void tbmqtt_update(void)
{
    struct tbmqtt_t *dev = &tbmqtt;
    struct statemachine_t *sm = &dev->sm;

    dev->txbuf_usage = (double)tbmqtt_get_txbuf_used() / (double)tbmqtt_get_txbuf_size() * 100;
    dev->rxbuf_usage = (double)tbmqtt_get_rxbuf_used() / (double)tbmqtt_get_rxbuf_size() * 100;
}

void tbmqtt_sm(void)
{
    struct tbmqtt_t *dev = &tbmqtt;
    struct statemachine_t *sm = &dev->sm;

    sm_cal_timing(sm);
    tbmqtt_update();

    switch (tbmqtt_get_state())
    {
    case SMST_OFFLINE:
        tbmqtt_sm_offline();
        break;
    case SMST_READY:
        tbmqtt_sm_ready();
        break;

    default:
        log_dbg("%s, never reach here", __func__);
        break;
    }
}