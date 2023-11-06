/*******************************************************************************
 * Copyright (c) 2012, 2020 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *   https://www.eclipse.org/legal/epl-2.0/
 * and the Eclipse Distribution License is available at
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *    Ian Craggs - add full capability
 *******************************************************************************/
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <uuid4.h>
#include "plt.h"

static mqtt_ringbuffer_t mqtt_txbuf;
static pthread_mutex_t mqtt_txbuf_mutex;
static mqtt_ringbuffer_t mqtt_rxbuf;
static pthread_mutex_t mqtt_rxbuf_mutex;

double mqtt_get_timeofday()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}

static void mqtt_connlost(void *context, char *cause)
{
    log_dbg("%s, mqtt connection lost, cause: %s\n", __func__, cause);
    mdl.mqtt.connlost = 1;

    /*
        mqtt_lock_txbuf();
        MQTTClient_destroy(&mqtt->cli);

        mqtt->reconncnt++;
        if(mqtt_connect()!=0){
            mqtt->connect = 0;
        }else{
            mqtt->connect = 1;
        }
        mqtt_unlock_txbuf();
    */
}

static int mqtt_msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    struct mqtt_t *dev = &mdl.mqtt;
    struct statemachine_t *sm = &mdl.mqtt.sm;
    int rc = 0;
    mqtt_ringbuffer_element_t e;
    char *pdst = NULL;
    char *psrc = NULL;
    int i;

    // log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName,topicLen, message->payloadlen);
    rc = MQTTClient_receive(dev->cli, &topicName, &topicLen, &message, 30);
    if (message)
    {
        log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName, topicLen, message->payloadlen);
        strcpy(e.sztopic, topicName);
        strncpy(e.szpayload, message->payload, message->payloadlen);
        log_dbg("%s, payload:%s", __func__, e.szpayload);
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        mqtt_lock_rxbuf();
        mqtt_queue_rxbuf(e);
        mqtt_unlock_rxbuf();
    }
}

static void mqtt_delivered(void *context, MQTTClient_deliveryToken dt)
{
    struct mqtt_t *dev = &mdl.mqtt;
    if (dev->dbg)
    {
        log_dbg("%s, Message with token value %d delivery confirmed", __func__, dt);
    }
}

extern char MQTTSERVIP[16];
extern int MQTTSERVPORT;
static int mqtt_connect(void)
{
    int ret = 0;
    int rc;
    char mqtt_server_url[256];
    struct mqtt_t *mqtt = &mdl.mqtt;
    int qos = 2;

    UUID4_STATE_T state;
    UUID4_T uuid;

    uuid4_seed(&state);
    uuid4_gen(&state, &uuid);

    char buf_uuid[UUID4_STR_BUFFER_SIZE];
    if (!uuid4_to_s(uuid, buf_uuid, sizeof(buf_uuid)))
    {
        ret = -1;
    }
    sprintf(mqtt->szclientid, "mdl-%d-%s", mdl.adr, buf_uuid);
    MQTTClient_connectOptions tmpconn_opts = MQTTClient_connectOptions_initializer;
    mqtt->conn_opts = tmpconn_opts;
    strcpy(mqtt->szservip, mdl.szmqtt_servip);
    mqtt->servport = mdl.mqtt_servport;
    // sprintf(buf,"tcp://%s:%d",mqtt->szservip,mqtt->servport);
    sprintf(mqtt_server_url, "%s:%d", mqtt->szservip, mqtt->servport);
    MQTTClient_create(&mqtt->cli, mqtt_server_url, mqtt->szclientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    mqtt->conn_opts.keepAliveInterval = 100;
    mqtt->conn_opts.cleansession = 1;
    // mqtt->conn_opts.username = mqtt->szaccesstoken;
    // mqtt->conn_opts.username = mqtt->szusername;
    // mqtt->conn_opts.password = mqtt->szpasswd;

    MQTTClient_setCallbacks(mqtt->cli, NULL, mqtt_connlost, NULL, NULL);
    if ((rc = MQTTClient_connect(mqtt->cli, &mqtt->conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        ret = -1;
    }

    sprintf(mqtt_server_url, "%s-ctl", mdl.szdev);
    rc = MQTTClient_subscribe(mqtt->cli, mqtt_server_url, qos);
    if (rc != MQTTCLIENT_SUCCESS && rc != qos)
    {
        ret = -2;
    }

    if (ret < 0)
    {
        log_dbg("%s, mqtt connect fail, return code %d\n", __func__, rc);
    }
    else
    {
        log_dbg("%s, mqtt connect ok, return code %d\n", __func__, rc);
    }
    return ret;
}

static int mqtt_get_state()
{
    return mdl.mqtt.sm.state;
}

static void mqtt_offline()
{
    struct mqtt_t *dev = &mdl.mqtt;
    char *szstate = dev->sm.szstate;
    int *step = &dev->sm.step;
    int *count = &dev->sm.count;
    int *cmd = &dev->cmd;

    if (*step == 0)
    { // wait cmd
        if (*cmd == CMD_SM_READY)
        { // ready cmd
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, get ready cmd, try to connect", __func__, szstate, *step);
            if (mqtt_connect() < 0)
            {
                if (dev->cli != NULL)
                {
                    MQTTClient_destroy(&dev->cli);
                }
                log_dbg("%s, state:%s, step:%d, connect fail, re-connet 10 seconds later", __func__, szstate, *step);
                *count = 0;
            }
            else
            {
                dev->connlost = 0;
                log_dbg("%s, state:%s, step:%d, connect ok, goto ready", __func__, szstate, *step);
                mqtt_set_state(SMST_READY, SMERR_NONE);
            }
        }
        else if ((*count)++ >= 200)
        { /* 10 seconds */
            *count = 0;
            log_dbg("%s, state:%s, step:%d, 10 seconds passed, try to connect", __func__, szstate, *step);
            if (mqtt_connect() < 0)
            {
                if (dev->cli != NULL)
                {
                    MQTTClient_destroy(&dev->cli);
                }
                log_dbg("%s, state:%s, step:%d, connect fail, re-connet 10 seconds later", __func__, szstate, *step);
                *count = 0;
            }
            else
            {
                dev->connlost = 0;
                log_dbg("%s, state:%s, step:%d, connect ok, goto ready", __func__, szstate, *step);
                mqtt_set_state(SMST_READY, SMERR_NONE);
            }
        }
    }
}

/*  tx ringbuffer handle  */
void mqtt_init_txbuf()
{
    mqtt_ringbuffer_init(&mqtt_txbuf);
}

void mqtt_lock_txbuf()
{
    pthread_mutex_lock(&mqtt_txbuf_mutex);
}

void mqtt_unlock_txbuf()
{
    pthread_mutex_unlock(&mqtt_txbuf_mutex);
}

void mqtt_queue_txbuf(mqtt_ringbuffer_element_t data)
{
    mqtt_ringbuffer_queue(&mqtt_txbuf, data);
}

int mqtt_dequeue_txbuf(mqtt_ringbuffer_element_t *data)
{
    return mqtt_ringbuffer_dequeue(&mqtt_txbuf, data);
}

int mqtt_peek_txbuf(mqtt_ringbuffer_element_t *data, mqtt_ringbuffer_size_t index)
{
    return mqtt_ringbuffer_peek(&mqtt_txbuf, data, 0);
}

/*  rx ringbuffer handle  */
void mqtt_init_rxbuf()
{
    mqtt_ringbuffer_init(&mqtt_rxbuf);
}

void mqtt_lock_rxbuf()
{
    pthread_mutex_lock(&mqtt_rxbuf_mutex);
}

void mqtt_unlock_rxbuf()
{
    pthread_mutex_unlock(&mqtt_rxbuf_mutex);
}

void mqtt_queue_rxbuf(mqtt_ringbuffer_element_t data)
{
    mqtt_ringbuffer_queue(&mqtt_rxbuf, data);
}

int mqtt_dequeue_rxbuf(mqtt_ringbuffer_element_t *data)
{
    return mqtt_ringbuffer_dequeue(&mqtt_rxbuf, data);
}

int mqtt_peek_rxbuf(mqtt_ringbuffer_element_t *data, mqtt_ringbuffer_size_t index)
{
    return mqtt_ringbuffer_peek(&mqtt_rxbuf, data, 0);
}

static int mqtt_pub(char *sztopic, char *szpayload)
{
    double pub_time;
    int ret = 0;
    int rc;
    struct mqtt_t *dev = &mdl.mqtt;
    MQTTClient_message msg = MQTTClient_message_initializer;

    dev->pub_starttime = mqtt_get_timeofday();
    msg.payload = szpayload;
    msg.payloadlen = (int)strlen(szpayload);
    msg.qos = 1;
    msg.retained = 0;
    MQTTClient_publishMessage(dev->cli, sztopic, &msg, &dev->token);
    // printf("Waiting for up to %d seconds for publication of %s\n"
    //         "on topic %s for client with ClientID: %s\n",
    //         (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(dev->cli, dev->token, 100000L);
    if (dev->dbg)
    {
        log_dbg("%s, Message with delivery token %d delivered,topic:%s\n",
                __func__, dev->token, dev->sztopic);
        log_dbg("%s,rc:%d\n", __func__, rc);
    }
    if (rc != 0)
    {
        ret = -2;
        goto leave;
    }
    else
    {
        dev->pub_endtime = mqtt_get_timeofday();
        pub_time = dev->pub_endtime - dev->pub_starttime;
        dev->pub_totalcnt += 1;
        dev->pub_totaltime += pub_time;
        dev->pub_ave = dev->pub_totaltime / dev->pub_totalcnt;
        if (pub_time > dev->pub_max)
        {
            dev->pub_max = pub_time;
        }
        if (dev->pub_totalcnt > 1000000)
        {
            dev->pub_totalcnt = 0.0;
            dev->pub_totaltime = 0.0;
            dev->pub_ave = 0.0;
            dev->pub_max = 0.0;
        }
    }

leave:
    if (ret != 0)
    {
        log_dbg("%s, ret:%d", __func__, ret);
    }
    return ret;
}

static void mqtt_ready()
{
    struct mqtt_t *dev = &mdl.mqtt;
    char *szstate = dev->sm.szstate;
    int *step = &dev->sm.step;
    int *count = &dev->sm.count;
    int *cmd = &dev->cmd;
    int rc = 0;
    mqtt_ringbuffer_element_t e;
    char *topicName = NULL;
    int topicLen;
    MQTTClient_message *message = NULL;
    char *pdst = NULL;
    char *psrc = NULL;
    int i;

    if (*step == 0)
    { // chk and wait cmd
        if (*cmd == CMD_SM_OFFLINE)
        { // offline cmd
            *cmd = CMD_SM_DONE;
            if (dev->cli != NULL)
            {
                MQTTClient_disconnect(dev->cli, 0);
                MQTTClient_destroy(&dev->cli);
            }
            log_dbg("%s, state:%s, step:%d, get offline cmd, goto offline", __func__, szstate, *step);
            mqtt_set_state(SMST_OFFLINE, SMERR_NONE);
        }
        else if (dev->connlost)
        {
            if (dev->cli != NULL)
            {
                MQTTClient_destroy(&dev->cli);
            }
            log_dbg("%s, state:%s, step:%d, connection lost detected, goto offline", __func__, szstate, *step);
            mqtt_set_state(SMST_OFFLINE, SMERR_NONE);
        }
        else
        {
#if 1
            rc = MQTTClient_receive(dev->cli, &topicName, &topicLen, &message, 50);
            if (message)
            {
                log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName, topicLen, message->payloadlen);
                pdst = e.szpayload;
                psrc = message->payload;
                for (i = 0; i < message->payloadlen; i++)
                {
                    *pdst++ = *psrc++;
                }
                *pdst = 0;
                log_dbg("%s, payload:%s", __func__, e.szpayload);
                MQTTClient_freeMessage(&message);
                MQTTClient_free(topicName);
                mqtt_lock_rxbuf();
                mqtt_queue_rxbuf(e);
                mqtt_unlock_rxbuf();
            }
            if (rc != 0)
            {
                dev->connlost = 1;
                log_dbg("%s, MQTTClient_receive fail:%d, set connlost=1", __func__, rc);
            }
#endif
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
            }
        }
    }
}

int mqtt_send_cmd(int cmd)
{
    mdl.mqtt.cmd = cmd;
    log_dbg("%s, cmd:%d", __func__, cmd);
}

static void mqtt_err()
{
    struct mqtt_t *dev = &mdl.mqtt;
    char *szstate = dev->sm.szstate;
    int *step = &dev->sm.step;
    int *cmd = &dev->cmd;

    if (*step == 0)
    {
        if (*cmd == CMD_SM_STDBY)
        {
            *cmd = CMD_SM_DONE;
            log_dbg("%s, state:%s, step:%d, get stdby cmd, goto stdby",
                    __func__, szstate, *step);
        }
    }
}

static void mqtt_sm(void)
{
    struct mqtt_t *dev = &mdl.mqtt;
    /* state machine timing statistics */
    double tseclipsed = 0.0;
    double ts = 0.0;
    if (dev->sm.timing_timer < 0)
    { /* reset */
        dev->sm.timing_timer = 0;
        ts = mqtt_get_timeofday();
        dev->sm.tslastrun = ts;
        dev->sm.totalcnt = 0;
        dev->sm.totaltime = 0;
        dev->sm.ave = -1.0;
        dev->sm.max = -1.0;
        dev->sm.cur = -1.0;
    }
    else
    {
        if (dev->sm.timing_timer++ >= 100)
        { /* cal every 100 times */
            dev->sm.timing_timer = 0;
            ts = mqtt_get_timeofday();
            tseclipsed = ts - dev->sm.tslastrun;
            dev->sm.totalcnt += 1;
            dev->sm.totaltime += tseclipsed;
            dev->sm.tslastrun = ts;
            dev->sm.ave = dev->sm.totaltime / dev->sm.totalcnt / 100;
            dev->sm.cur = tseclipsed / 100;
            if (dev->sm.cur > dev->sm.max)
            {
                dev->sm.max = dev->sm.cur;
            }
            if (dev->sm.totalcnt > 100000)
            { /* auto reset */
                dev->sm.timing_timer = -1;
            }
        }
    }
    if (dev->sm.tick_timer++ >= 20)
    { /* 1s */
        dev->sm.tick_timer = 0;
        dev->sm.tick++;
    }

    dev->txbuf_usage = (double)mqtt_get_txbuf_used() / (double)mqtt_get_txbuf_size() * 100;
    dev->rxbuf_usage = (double)mqtt_get_rxbuf_used() / (double)mqtt_get_rxbuf_size() * 100;

    switch (mqtt_get_state())
    {
    case SMST_OFFLINE:
        mqtt_offline();
        break;

    case SMST_READY:
        mqtt_ready();
        break;

    default:
        log_dbg("%s, never reach here", __func__);
        break;
    }
}

static void mqtt_thrd(void *param)
{

    log_dbg("%s, ++", __func__);

    while (1)
    {
        mqtt_sm();
        usleep(50000); /* 50ms */
    }

    log_dbg("%s, --", __func__);
}

static int mqtt_dbcb_0(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    int i;
    struct dbcbparam_t *pcbparam = (struct dbcbparam_t *)para;
    struct mqtt_t *dev = &mdl.mqtt;

    pcbparam->nrow++;
    log_dbg("%s, ++,row:%d, col:%d", __func__, pcbparam->nrow, ncolumn);

    if (pcbparam->nrow > 1)
    {
        log_dbg("%s, mqtt cfg rows is more than 1!", __func__);
        return -1;
    }

    for (i = 0; i < ncolumn; i++)
    {
        if (strcmp("servip", columnname[i]) == 0)
        {
            strcpy(dev->szservip, columnvalue[i]);
        }
        else if (strcmp("servport", columnname[i]) == 0)
        {
            dev->servport = atoi(columnvalue[i]);
        }
        else if (strcmp("username", columnname[i]) == 0)
        {
            strcpy(dev->szusername, columnvalue[i]);
        }
        else if (strcmp("clientid", columnname[i]) == 0)
        {
            strcpy(dev->szclientid, columnvalue[i]);
        }
        else if (strcmp("passwd", columnname[i]) == 0)
        {
            strcpy(dev->szpasswd, columnvalue[i]);
        }
        else if (strcmp("datatopic", columnname[i]) == 0)
        {
            strcpy(dev->szdatatopic, columnvalue[i]);
        }
        else if (strcmp("cmdtopic", columnname[i]) == 0)
        {
            strcpy(dev->szcmdtopic, columnvalue[i]);
        }
    }

    pcbparam->ret = 0;
    log_dbg("%s, --,ret:%d", __func__, pcbparam->ret);
    return 0;
}

int mqtt_set_state(int state, int errcode)
{
    struct mqtt_t *dev = &mdl.mqtt;
    dev->sm.state = state;
    dev->sm.errcode = errcode;
    dev->sm.step = 0;
    dev->sm.count = 0;

    switch (state)
    {
    case SMST_OFFLINE:
        strcpy(dev->sm.szstate, "offline");
        break;

    case SMST_READY:
        strcpy(dev->sm.szstate, "ready");
        break;

    default:
        strcpy(dev->sm.szstate, "unknown");
        break;
    }

    switch (errcode)
    {
    case SMERR_NONE:
        strcpy(dev->sm.szerrcode, "none");
        break;

    case SMERR_NA:
        strcpy(dev->sm.szerrcode, "na");
        break;

    default:
        strcpy(dev->sm.szerrcode, "unknown");
        break;
    }
}

int mqtt_init()
{
    pthread_t xthrd;
    int rc = 0;
    int ret = 0;
    struct mqtt_t *dev = &mdl.mqtt;

    log_dbg("%s, ++", __func__);

    dev->sm.tslastrun = -1; /* reset sm timing */
    dev->cmd = CMD_SM_READY;
    mqtt_set_state(SMST_OFFLINE, SMERR_NONE);
    dev->sm.timing_timer = -1;
    /* reset pub timings */
    dev->pub_totalcnt = 0.0;
    dev->pub_totaltime = 0.0;
    dev->pub_ave = 0.0;
    dev->pub_max = 0.0;
    mqtt_init_txbuf();
    mqtt_init_rxbuf();
    pthread_mutex_init(&mqtt_txbuf_mutex, NULL);
    pthread_mutex_init(&mqtt_rxbuf_mutex, NULL);

    if (pthread_create(&xthrd, NULL, mqtt_thrd, NULL) != 0)
    {
        log_dbg("%s, create mqtt_thrd fail", __func__);
        ret = -1;
    }

    log_dbg("%s--, ret:%d", __func__, ret);
    return ret;
}

int mqtt_get_txbuf_used(void)
{
    return mqtt_ringbuffer_num_items(&mqtt_txbuf);
}

int mqtt_get_txbuf_size(void)
{
    return mqtt_ringbuffer_size(&mqtt_txbuf);
}

int mqtt_get_rxbuf_used(void)
{
    return mqtt_ringbuffer_num_items(&mqtt_rxbuf);
}

int mqtt_get_rxbuf_size(void)
{
    return mqtt_ringbuffer_size(&mqtt_rxbuf);
}