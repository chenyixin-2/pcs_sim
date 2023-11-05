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

#include "plt.h"
#include "tbmqtt_cache.h"
#define TBMQTT_CACHE_DATA_BASE "../data/tbmqtt.db"
#define TBMQTT_CACHE_TABLE "tbmqtt_cache"
#define TX_BUFF_TO_CACHE_USAGE (75.0)
#define CACHE_TO_TX_BUFF_USAGE (65.0)
#define CACHE_FILE_MEMORY_SIZE_MAX (40 * 1024 * 1024)

static tbmqtt_ringbuffer_t tbmqtt_txbuf;
static pthread_mutex_t tbmqtt_txbuf_mutex;
static tbmqtt_ringbuffer_t tbmqtt_rxbuf;
static pthread_mutex_t tbmqtt_rxbuf_mutex;

static void *tbmqtt_cache_handle = NULL;

double tbmqtt_get_timeofday()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}

static void tbmqtt_connlost(void *context, char *cause)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    log_dbg("%s, mqtt connection lost, cause: %s\n", __func__, cause);
    dev->connlost = 1;

    /*
        tbmqtt_lock_txbuf();
        MQTTClient_destroy(&mqtt->cli);

        mqtt->reconncnt++;
        if(tbmqtt_connect()!=0){
            mqtt->connect = 0;
        }else{
            mqtt->connect = 1;
        }
        tbmqtt_unlock_txbuf();
    */
}

static int tbmqtt_msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    struct statemachine_t *sm = &dev->sm;
    int rc = 0;
    tbmqtt_ringbuffer_element_t e;
    char *pdst = NULL;
    char *psrc = NULL;
    int i;

    // log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName,topicLen, message->payloadlen);
    if (message)
    {
        pdst = e.sztopic;
        psrc = topicName;
        for (i = 0; i < topicLen; i++)
        {
            *pdst++ = *psrc++;
        }
        *pdst = 0;
        pdst = e.szpayload;
        psrc = message->payload;
        for (i = 0; i < message->payloadlen; i++)
        {
            *pdst++ = *psrc++;
        }
        *pdst = 0;
        if (dev->dbg)
        {
            log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName, topicLen, message->payloadlen);
            log_dbg("%s, payload:%s", __func__, e.szpayload);
        }
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        tbmqtt_lock_rxbuf();
        tbmqtt_queue_rxbuf(e);
        tbmqtt_unlock_rxbuf();
    }
}

// static int tbmqtt_msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
// {

//     int i;
//     char* topicptr;
//     char topicbuf[1024];
//     char plbuf[1024];
//     char* payloadptr;
//     char szreqid[256];
//     char* pszreqid = szreqid;
//     char* p = NULL;
//     int reqid = 0;
//     tbmqtt_ringbuffer_element_t e;

//     /*
//     p = plbuf;
//     payloadptr = message->payload;
//     for( i = 0; i < message->payloadlen; i++ ){
//         *p++ = *payloadptr++;
//     }
//     *p = 0;
//     p = topicbuf;
//     topicptr = topicName;
//     for( i = 0; i < topicLen; i++ ){
//         *p++ = *topicptr++;
//     }
//     *p = 0;
//     */
//     log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d",
//             __func__, topicbuf,topicLen, message->payloadlen);

//     #if 0
//     if( topicLen > 0 ){
//         p = strrchr(topicName, '/');
//         if( p != NULL ){
//             p++;
//             while( *p != 0){
//                 *pszreqid++ = *p++;
//             }
//             *pszreqid = 0;
//             reqid = atoi(szreqid);
//             log_dbg("%s, requestid:%s|%d", __func__, szreqid, reqid);

//             e.cmd = CMD_MQTT_SENDKV;
//             e.sztopic[0] = 0;
//             e.szpayload[0] = 0;
//             strcpy(e.szpayload, plbuf);
//             sprintf(e.sztopic,"v1/devices/me/rpc/response/%s",szreqid);

//             tbmqtt_lock_txbuf();
//             tbmqtt_ringbuffer_queue(tbmqtt_rb, e);
//             tbmqtt_unlock_txbuf();

//         }
//     }
//     #endif

//     MQTTClient_freeMessage(&message);
//     MQTTClient_free(topicName);

//     return 0;
// }

static void tbmqtt_delivered(void *context, MQTTClient_deliveryToken dt)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    if (dev->dbg)
    {
        log_dbg("%s, Message with token value %d delivery confirmed", __func__, dt);
    }
}

int tbmqtt_connect(void)
{
    int ret = 0;
    int rc;
    char buf[256];
    struct tbmqtt_t *mqtt = &mdl.tbmqtt;
    int qos = 2;

    MQTTClient_connectOptions tmpconn_opts = MQTTClient_connectOptions_initializer;
    mqtt->conn_opts = tmpconn_opts;

    sprintf(buf, "tcp://%s:%d", mqtt->szservip, mqtt->servport);
    MQTTClient_create(&mqtt->cli, buf, mqtt->szclientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    mqtt->conn_opts.keepAliveInterval = 100;
    mqtt->conn_opts.cleansession = 1;
    mqtt->conn_opts.username = mqtt->szaccesstoken;
    // mqtt->conn_opts.username = mqtt->szusername;
    // mqtt->conn_opts.password = mqtt->szpasswd;

    MQTTClient_setCallbacks(mqtt->cli, NULL, tbmqtt_connlost, tbmqtt_msgarrvd, NULL);
    if ((rc = MQTTClient_connect(mqtt->cli, &mqtt->conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        log_dbg("%s, MQTTClient_connect fail:%d", __func__, rc);
        ret = -1;
        goto leave;
    }

    // sprintf(buf, "ems/data/EssCmd/%s", sta_get_sn());
    rc = MQTTClient_subscribe(mqtt->cli, "v1/devices/me/rpc/request/+", qos);
    if (rc != MQTTCLIENT_SUCCESS && rc != qos)
    {
        log_dbg("%s, MQTTClient_subscribe fail:%d", __func__, rc);
        ret = -2;
        goto leave;
    }

leave:
    return ret;
}

/*  tx ringbuffer handle  */
void tbmqtt_init_txbuf()
{
    tbmqtt_ringbuffer_init(&tbmqtt_txbuf);
}

void tbmqtt_lock_txbuf()
{
    pthread_mutex_lock(&tbmqtt_txbuf_mutex);
}

void tbmqtt_unlock_txbuf()
{
    pthread_mutex_unlock(&tbmqtt_txbuf_mutex);
}

void tbmqtt_queue_txbuf(tbmqtt_ringbuffer_element_t data)
{
    tbmqtt_ringbuffer_queue(&tbmqtt_txbuf, data);
}

int tbmqtt_dequeue_txbuf(tbmqtt_ringbuffer_element_t *data)
{
    return tbmqtt_ringbuffer_dequeue(&tbmqtt_txbuf, data);
}

int tbmqtt_peek_txbuf(tbmqtt_ringbuffer_element_t *data, tbmqtt_ringbuffer_size_t index)
{
    return tbmqtt_ringbuffer_peek(&tbmqtt_txbuf, data, 0);
}

/*  rx ringbuffer handle  */
void tbmqtt_init_rxbuf()
{
    tbmqtt_ringbuffer_init(&tbmqtt_rxbuf);
}

void tbmqtt_lock_rxbuf()
{
    pthread_mutex_lock(&tbmqtt_rxbuf_mutex);
}

void tbmqtt_unlock_rxbuf()
{
    pthread_mutex_unlock(&tbmqtt_rxbuf_mutex);
}

void tbmqtt_queue_rxbuf(tbmqtt_ringbuffer_element_t data)
{
    tbmqtt_ringbuffer_queue(&tbmqtt_rxbuf, data);
}

int tbmqtt_dequeue_rxbuf(tbmqtt_ringbuffer_element_t *data)
{
    return tbmqtt_ringbuffer_dequeue(&tbmqtt_rxbuf, data);
}

int tbmqtt_peek_rxbuf(tbmqtt_ringbuffer_element_t *data, tbmqtt_ringbuffer_size_t index)
{
    return tbmqtt_ringbuffer_peek(&tbmqtt_rxbuf, data, 0);
}

int tbmqtt_pub(char *sztopic, char *szpayload)
{
    double pub_time;
    int ret = 0;
    int rc;
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    MQTTClient_message msg = MQTTClient_message_initializer;

    dev->pub_starttime = tbmqtt_get_timeofday();
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
        log_info("%s, Message with delivery token %d delivered,topic:%s",
                 __func__, dev->token, sztopic);
        log_info("%s,rc:%d payload:%s", __func__, rc, szpayload);
    }
    if (rc != 0)
    {
        ret = -2;
        goto leave;
    }
    else
    {
        dev->pub_endtime = tbmqtt_get_timeofday();
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

int tbmqtt_send_sm_cmd(int cmd)
{
    mdl.tbmqtt.cmd = cmd;
    log_dbg("%s, cmd:%d", __func__, cmd);
}

static void tbmqtt_deal_with_cache(void)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    double *txbuf_usage = &dev->txbuf_usage;
    tbmqtt_ringbuffer_element_t e;
    int rc = 0;
    int cache_item_nb = 0;
    int cache_item_idx = 0;
    static int count = 0;

    if (tbmqtt_cache_handle == NULL)
    {
        return;
    }

    if (*txbuf_usage > TX_BUFF_TO_CACHE_USAGE)
    {

        // get file memory size
        long cache_memroy_size = 0;
        tbmqtt_cache_get_memory_size(tbmqtt_cache_handle, &cache_memroy_size);
        if (cache_memroy_size > CACHE_FILE_MEMORY_SIZE_MAX)
        {
            return;
        }

        tbmqtt_lock_txbuf();
        rc = tbmqtt_peek_txbuf(&e, 0);
        tbmqtt_unlock_txbuf();
        if (rc == 1)
        {
            // write to cache
            if (tbmqtt_cache_write_one_payload(tbmqtt_cache_handle, TBMQTT_CACHE_TABLE, e) == 0)
            { // if success,delet from buf
                tbmqtt_lock_txbuf();
                tbmqtt_dequeue_txbuf(&e);
                tbmqtt_unlock_txbuf();
            }
        }
    }
    else if (*txbuf_usage < CACHE_TO_TX_BUFF_USAGE)
    {
        rc = tbmqtt_cache_get_payload_nb(tbmqtt_cache_handle, TBMQTT_CACHE_TABLE, &cache_item_nb);
        if (cache_item_nb > 0 && rc == 0)
        { // have payload in cache
            // read one payload
            e.sztopic[0] = 0;
            e.szpayload[0] = 0;
            tbmqtt_cache_read_one_payload(tbmqtt_cache_handle, TBMQTT_CACHE_TABLE, &e, &cache_item_idx);
            e.cmd = CMD_MQTT_SENDKV;
            // add to txbuf
            tbmqtt_lock_txbuf();
            tbmqtt_queue_txbuf(e);
            tbmqtt_unlock_txbuf();
            // delete from cache
            tbmqtt_cache_delete_one_payload(tbmqtt_cache_handle, TBMQTT_CACHE_TABLE, cache_item_idx);
            count++;
            // free memory
            if (count > 500)
            {
                count = 0;
                tbmqtt_cache_free_memory(tbmqtt_cache_handle);
            }
        } // else have no data in cache
    }
    else
    {
        // do nothing
    }
}

static void tbmqtt_db_cache_thrd_main(void *param)
{
    log_dbg("%s, ++", __func__);
    // open or cread cache table
    tbmqtt_cache_open(TBMQTT_CACHE_DATA_BASE, TBMQTT_CACHE_TABLE, &tbmqtt_cache_handle);
    while (1)
    {
        tbmqtt_deal_with_cache();
        usleep(10000); /* 10ms */
    }
    log_dbg("%s, --", __func__);
}

static void tbmqtt_update(void)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;

    dev->txbuf_usage = (double)tbmqtt_get_txbuf_used() / (double)tbmqtt_get_txbuf_size() * 100;
    dev->rxbuf_usage = (double)tbmqtt_get_rxbuf_used() / (double)tbmqtt_get_rxbuf_size() * 100;
}

static void tbmqtt_sm_offline()
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    struct statemachine_t *sm = &dev->sm;
    int *state = &dev->sm.state;
    char *szstate = dev->sm.szstate;
    int *step = &dev->sm.step;
    int *count = &dev->sm.count;
    int *cmd = &dev->cmd;

    if (*step == 0)
    { // entry
        log_dbg("%s, state:%s, step:%d, entry", __func__, szstate, *step);
        *cmd = CMD_SM_READY;
        *step = 10;
    }
    else if (*step == 10)
    { // wait cmd
        if (*cmd == CMD_SM_READY)
        { // ready cmd
            log_dbg("%s, state:%s, step:%d, get ready cmd, try to connect", __func__, szstate, *step);
            *cmd = CMD_SM_DONE;
            if (tbmqtt_connect() < 0)
            {
                log_dbg("%s, state:%s, step:%d, tbmqtt connect fail, re-connect 10 seconds later", __func__, szstate, *step);
                if (dev->cli != NULL)
                {
                    MQTTClient_destroy(&dev->cli);
                }
                *count = 0;
            }
            else
            {
                dev->connlost = 0;
                log_dbg("%s, state:%s, step:%d, connect ok, goto ready", __func__, szstate, *step);
                tbmqtt_set_state(SMST_READY, SMERR_NONE);
            }
        }
        else
        {
            (*step)++;
            if (*count >= 1000)
            { // 10s
                *count = 0;
                if (dev->enable)
                {
                    log_dbg("%s, state:%s, step:%d, 10 seconds passed, try to connect", __func__, szstate, *step);
                    if (tbmqtt_connect() < 0)
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
                        tbmqtt_set_state(SMST_READY, SMERR_NONE);
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
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    struct statemachine_t *sm = &dev->sm;
    int *state = &dev->sm.state;
    char *szstate = dev->sm.szstate;
    int *step = &dev->sm.step;
    int *count = &dev->sm.count;
    int *cmd = &dev->cmd;
    int rc = 0;
    tbmqtt_ringbuffer_element_t e;
    char *topicName = NULL;
    int topicLen;
    MQTTClient_message *message = NULL;
    char *pdst = NULL;
    char *psrc = NULL;
    int i;

    if (*step == 0)
    { // entry
        log_dbg("%s, state:%s, step:%d, entry", __func__, szstate, *step);
        *cmd = CMD_SM_DONE;
        *step = 10;
    }
    else if (*step == 10)
    { // wait and chk
        if (*cmd == CMD_SM_OFFLINE)
        { // offline cmd
            log_dbg("%s, state:%s, step:%d, get offline cmd, goto offline", __func__, szstate, *step);
            *cmd = CMD_SM_DONE;
            if (dev->cli != NULL)
            {
                MQTTClient_disconnect(dev->cli, 0);
                MQTTClient_destroy(&dev->cli);
            }
            tbmqtt_set_state(SMST_OFFLINE, SMERR_NONE);
        }
        else if (dev->connlost)
        {
            log_dbg("%s, state:%s, step:%d, connection lost detected, goto offline", __func__, szstate, *step);
            if (dev->cli != NULL)
            {
                MQTTClient_destroy(&dev->cli);
            }
            tbmqtt_set_state(SMST_OFFLINE, SMERR_NONE);
        }
        else
        {
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
                    tbmqtt_lock_txbuf();
                    tbmqtt_dequeue_txbuf(&e);
                    tbmqtt_unlock_txbuf();
                }
            }
        }
    }
}

static void tbmqtt_sm(void)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    struct statemachine_t *sm = &dev->sm;

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

static void tbmqtt_thrd(void *param)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    struct statemachine_t *sm = &dev->sm;
    pthread_t xthrd;
    log_dbg("%s, ++", __func__);

    /* reset pub timings */
    dev->pub_totalcnt = 0.0;
    dev->pub_totaltime = 0.0;
    dev->pub_ave = 0.0;
    dev->pub_max = 0.0;
    tbmqtt_init_txbuf();
    tbmqtt_init_rxbuf();
    pthread_mutex_init(&tbmqtt_txbuf_mutex, NULL);
    pthread_mutex_init(&tbmqtt_rxbuf_mutex, NULL);

    // open or cread cache table
    // if(pthread_create(&xthrd,NULL, tbmqtt_db_cache_thrd_main, NULL)!=0){
    //     log_dbg( "%s, create tbmqtt_db_cache_thrd_main fail", __func__);
    // }
    tbmqtt_set_state(SMST_OFFLINE, SMERR_NONE);

    while (1)
    {
        tbmqtt_sm();
        // tbmqtt_deal_with_cache();
        usleep(50000); /* 50ms */
    }

    log_dbg("%s, --", __func__);
}

int tbmqtt_set_dbg(int val)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
    dev->dbg = val;
    return 0;
}

int tbmqtt_get_state()
{
    return mdl.tbmqtt.sm.state;
}

int tbmqtt_set_state(int state, int errcode)
{
    struct tbmqtt_t *dev = &mdl.tbmqtt;
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

    case SMST_ERR:
        strcpy(dev->sm.szstate, "err");
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

static int tbmqtt_dbcb_0(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    int i;
    struct dbcbparam_t *pcbparam = (struct dbcbparam_t *)para;
    struct tbmqtt_t *dev = &mdl.tbmqtt;

    pcbparam->nrow++;
    log_dbg("%s, ++,row:%d, col:%d", __func__, pcbparam->nrow, ncolumn);

    if (pcbparam->nrow > 1)
    {
        log_dbg("%s, tbmqtt cfg rows is more than 1!", __func__);
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
        else if (strcmp("clientid", columnname[i]) == 0)
        {
            strcpy(dev->szclientid, columnvalue[i]);
        }
        else if (strcmp("accesstoken", columnname[i]) == 0)
        {
            strcpy(dev->szaccesstoken, columnvalue[i]);
        }
    }

    pcbparam->ret = 0;
    log_dbg("%s, --,ret:%d", __func__, pcbparam->ret);
    return 0;
}

int tbmqtt_reset()
{
    tbmqtt_lock_txbuf();
    tbmqtt_init_rxbuf();
    tbmqtt_init_txbuf();
    tbmqtt_unlock_txbuf();
}

int tbmqtt_init()
{
    pthread_t xthrd;
    int rc = 0;
    int ret = 0;
    char *errmsg = NULL;
    char sql[1024];
    struct dbcbparam_t cbparam;
    sqlite3 *db = NULL;

    log_dbg("%s, ++", __func__);

    plt_lock_projdb();
    db = plt_get_projdb();
    sprintf(sql, "select * from tbmqtt_sta");
    cbparam.nrow = 0;
    rc = sqlite3_exec(db, sql, tbmqtt_dbcb_0, (void *)&cbparam, &errmsg);
    plt_unlock_projdb();
    if (rc != SQLITE_OK)
    {
        ret = -1;
    }
    else if (cbparam.ret != 0)
    {
        ret = -2;
    }
    else
    {
        if (pthread_create(&xthrd, NULL, tbmqtt_thrd, NULL) != 0)
        {
            log_dbg("%s, create tbmqtt thrd fail", __func__);
            ret = -1;
        }
    }

    log_dbg("%s--, ret:%d", __func__, ret);
    return ret;
}

int tbmqtt_get_txbuf_used(void)
{
    return tbmqtt_ringbuffer_num_items(&tbmqtt_txbuf);
}

int tbmqtt_get_txbuf_size(void)
{
    return tbmqtt_ringbuffer_size(&tbmqtt_txbuf);
}

int tbmqtt_get_rxbuf_used(void)
{
    return tbmqtt_ringbuffer_num_items(&tbmqtt_rxbuf);
}

int tbmqtt_get_rxbuf_size(void)
{
    return tbmqtt_ringbuffer_size(&tbmqtt_rxbuf);
}
