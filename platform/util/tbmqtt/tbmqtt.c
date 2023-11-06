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
#include "plt.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "tbmqtt_sm.h"
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

struct tbmqtt_t tbmqtt;

double tbmqtt_get_timeofday()
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
}

static void tbmqtt_connlost(void *context, char *cause)
{
    struct tbmqtt_t *dev = &tbmqtt;

    log_dbg("%s, mqtt connection lost, cause: %s\n", __func__, cause);
    dev->connlost = 1;
    dev->connLostCnt++;

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
    struct tbmqtt_t *dev = &tbmqtt;
    struct statemachine_t *sm = &dev->sm;
    int rc = 0;
    tbmqtt_ringbuffer_element_t e;
    char *pdst = NULL;
    char *psrc = NULL;
    int i;

    // log_dbg("%s, Message arrived, topic:%s topic len:%d payload len:%d", __func__, topicName,topicLen, message->payloadlen);
    if (message)
    {
        strcpy(e.sztopic, topicName);
        strncpy(e.szpayload, (const char*)message->payload, message->payloadlen);

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
    struct tbmqtt_t *dev = &tbmqtt;
    if (dev->dbg)
    {
        log_dbg("%s, Message with token value %d delivery confirmed", __func__, dt);
    }
}

int tbmqtt_connect(void)
{
    int ret = 0;
    int rc;
    char buf[1024];
    struct tbmqtt_t *mqtt = &tbmqtt;
    int qos = 2;

    MQTTClient_connectOptions tmpconn_opts = MQTTClient_connectOptions_initializer;
    mqtt->conn_opts = tmpconn_opts;

    sprintf(buf, "tcp://%s:%d", mqtt->szservip, mqtt->servport);
    if ((rc = MQTTClient_create(&mqtt->cli, buf, mqtt->szclientid, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTCLIENT_SUCCESS)
    {
        log_dbg("%s, MQTTClient_create fail:%s", __func__, MQTTClient_strerror(rc));
        ret = -1;
        goto leave;
    }

    mqtt->conn_opts.keepAliveInterval = 100;
    mqtt->conn_opts.cleansession = 1;
    mqtt->conn_opts.username = mqtt->szaccesstoken;
    // mqtt->conn_opts.username = mqtt->szusername;
    // mqtt->conn_opts.password = mqtt->szpasswd;

    MQTTClient_setCallbacks(mqtt->cli, NULL, tbmqtt_connlost, tbmqtt_msgarrvd, NULL);
    if ((rc = MQTTClient_connect(mqtt->cli, &mqtt->conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        log_dbg("%s, MQTTClient_connect fail:%s", __func__, MQTTClient_strerror(rc));
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

int tbmqtt_get_state()
{
    struct tbmqtt_t *dev = &tbmqtt;
    return dev->sm.state;
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
    struct tbmqtt_t *dev = &tbmqtt;

    if (dev->dbg)
    {
        log_dbg("%s Message with delivery token:%d, topic:%s, payload:%s\n",
                __func__, dev->token, sztopic, szpayload);
    }

    MQTTClient_message msg = MQTTClient_message_initializer;
    dev->pub_starttime = tbmqtt_get_timeofday();
    msg.payload = szpayload;
    msg.payloadlen = (int)strlen(szpayload);
    msg.qos = 1;
    msg.retained = 0;
#ifdef DEBUG_MQTT
    log_dbg("Publish");
#endif
    rc = MQTTClient_publishMessage(dev->cli, sztopic, &msg, &dev->token);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_dbg("%s, MQTTClient_publisMessage fail:%s", __func__, MQTTClient_strerror(rc));
        dev->pub_failed++;
        ret = -1;
        goto leave;
    }
    // printf("Waiting for up to %d seconds for publication of %s\n"
    //         "on topic %s for client with ClientID: %s\n",
    //         (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
#ifdef DEBUG_MQTT
    log_dbg("Wait for completion");
#endif
    rc = MQTTClient_waitForCompletion(dev->cli, dev->token, 100000L);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_dbg("%s, MQTTClient_waitForCompletion fail:%s", __func__, MQTTClient_strerror(rc));
        dev->pub_failed++;
        ret = -2;
        goto leave;
    }
    else
    {
#ifdef DEBUG_MQTT
        log_dbg("Done");
#endif
        dev->pub_endtime = tbmqtt_get_timeofday();
        pub_time = dev->pub_endtime - dev->pub_starttime;
        dev->pubTotalCnt += 1;
        dev->pubTotalTime += pub_time;
        dev->pubAvg = dev->pubTotalTime / dev->pubTotalCnt;
        if (pub_time > dev->pubMax)
        {
            dev->pubMax = pub_time;
        }
        if (dev->pubTotalCnt > 1000000)
        {
            dev->pubTotalCnt = 0.0;
            dev->pubTotalTime = 0.0;
            dev->pubAvg = 0.0;
            dev->pubMax = 0.0;
        }
        if (dev->pub_failed > dev->pub_maxFailcnt)
        {
            dev->pub_maxFailcnt = dev->pub_failed;
        }
        dev->pub_totalFailcnt += dev->pub_failed;
        dev->pub_failed = 0;
    }

leave:
    if (dev->dbg && ret != 0)
    {
        log_dbg("%s, ret:%d", __func__, ret);
    }
    return ret;
}

int tbmqtt_send_sm_cmd(int cmd)
{
    tbmqtt.cmd = cmd;
    log_dbg("%s, cmd:%d", __func__, cmd);
}

static void tbmqtt_deal_with_cache(void)
{
    struct tbmqtt_t *dev = &tbmqtt;
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

static void *tbmqtt_thrd_main(void *param)
{
    struct tbmqtt_t *dev = &tbmqtt;
    struct statemachine_t *sm = &dev->sm;
    pthread_t xthrd;
    log_dbg("%s, ++", __func__);

    /* reset pub timings */
    dev->connLostCnt = 0;
    dev->pubTotalCnt = 0.0;
    dev->pubTotalTime = 0.0;
    dev->pubAvg = 0.0;
    dev->pubMax = 0.0;
    tbmqtt_init_txbuf();
    tbmqtt_init_rxbuf();
    pthread_mutex_init(&tbmqtt_txbuf_mutex, NULL);
    pthread_mutex_init(&tbmqtt_rxbuf_mutex, NULL);

    // open or cread cache table
    // if(pthread_create(&xthrd,NULL, tbmqtt_db_cache_thrd_main, NULL)!=0){
    //     log_dbg( "%s, create tbmqtt_db_cache_thrd_main fail", __func__);
    // }
    tbmqtt_sm_init();

    while (1)
    {
        tbmqtt_sm();
        // tbmqtt_deal_with_cache();
        usleep(10000); /* 10ms */
    }

    log_dbg("%s, --", __func__);
    return NULL;
}

int tbmqtt_set_dbg(int val)
{
    struct tbmqtt_t *dev = &tbmqtt;
    dev->dbg = val;
    return 0;
}

static int tbmqtt_dbcb_0(void *para, int ncolumn, char **columnvalue, char *columnname[])
{
    int i;
    struct dbcbparam_t *pcbparam = (struct dbcbparam_t *)para;
    struct tbmqtt_t *dev = &tbmqtt;

    pcbparam->nrow++;
    log_dbg("%s, ++,row:%d, col:%d", __func__, pcbparam->nrow, ncolumn);

    if (pcbparam->nrow > 1)
    {
        log_dbg("%s, tbmqtt cfg rows is more than 1!", __func__);
        return -1;
    }

    for (i = 0; i < ncolumn; i++)
    {
        if (strcmp("enable", columnname[i]) == 0)
        {
            dev->enable = atoi(columnvalue[i]);
        }
        else if (strcmp("servip", columnname[i]) == 0)
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
        else if (strcmp("username", columnname[i]) == 0)
        {
            strcpy(dev->szusername, columnvalue[i]);
        }
        else if (strcmp("passwd", columnname[i]) == 0)
        {
            strcpy(dev->szpasswd, columnvalue[i]);
        }
        else if (strcmp("accesstoken", columnname[i]) == 0)
        {
            strcpy(dev->szaccesstoken, columnvalue[i]);
        }
        else if (strcmp("timezone", columnname[i]) == 0)
        {
            dev->timezone = atoi(columnvalue[i]);
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

int tbmqtt_get_cmd()
{
    return tbmqtt.cmd;
}

void tbmqtt_reset_cmd()
{
    tbmqtt.cmd = CMD_SM_DONE;
}

int tbmqtt_init()
{
    pthread_t xthrd;
    int rc = 0;
    int ret = 0;
    struct tbmqtt_t *dev = &tbmqtt;
    char *errmsg = NULL;
    char sql[1024];
    struct dbcbparam_t cbparam;
    sqlite3 *db = NULL;

    log_dbg("%s, ++", __func__);

    plt_lock_projdb();
    db = plt_get_projdb();
    sprintf(sql, "select * from tbmqtt");
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
        if (pthread_create(&xthrd, NULL, tbmqtt_thrd_main, NULL) != 0)
        {
            log_dbg("%s, create tbmqtt_thrd_sm fail", __func__);
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

int tbmqtt_get_tz()
{
    return tbmqtt.timezone;
}

char *tbmqtt_get_state_str(void)
{
    return tbmqtt.sm.szstate;
}

int tbmqtt_get_stp(void)
{
    return tbmqtt.sm.step;
}

char *tbmqtt_get_err_str(void)
{
    return tbmqtt.sm.szerrcode;
}

int tbmqtt_get_tick(void)
{
    return tbmqtt.sm.tick;
}

double tbmqtt_get_timing_ave(void)
{
    return tbmqtt.sm.timing_ave;
}

double tbmqtt_get_timing_cur(void)
{
    return tbmqtt.sm.timing_cur;
}

double tbmqtt_get_timing_max(void)
{
    return tbmqtt.sm.timing_max;
}

int tbmqtt_get_enable(void)
{
    return tbmqtt.enable;
}

char *tbmqtt_get_servip_str(void)
{
    return tbmqtt.szservip;
}

int tbmqtt_get_servport(void)
{
    return tbmqtt.servport;
}

char *tbmqtt_get_client_id(void)
{
    return tbmqtt.szclientid;
}

double tbmqtt_get_txbuf_usage(void)
{
    return tbmqtt.txbuf_usage;
}

char *tbmqtt_get_access_token(void)
{
    return tbmqtt.szaccesstoken;
}

int tbmqtt_get_tool_data(char *buf)
{
    struct tbmqtt_t *dev = &tbmqtt;
    struct statemachine_t *sm = &dev->sm;

    char buf_temp[8192];

    sprintf(buf, " TBMQTT ");

    sm_get_summary(sm, buf_temp, sizeof(buf_temp));
    strcat(buf, buf_temp);

    sprintf(buf_temp, "   en:%d tz:%d servip:%s servport:%d clientid:%s accesstoken:%s txbufusage:%.1f rxbufusage:%.1f lost count:%d \n",
            dev->enable, dev->timezone, dev->szservip, dev->servport, dev->szclientid, dev->szaccesstoken, dev->txbuf_usage, dev->rxbuf_usage, dev->connLostCnt);
    strcat(buf, buf_temp);

    sprintf(buf_temp, " dbg:%d", dev->dbg);
    strcat(buf, buf_temp);
    return 0;
}
