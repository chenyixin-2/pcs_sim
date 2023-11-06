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

#include <mqtt_sm.h>

#define MQTT_CACHE_DATA_BASE "../data/mqtt.db"
#define MQTT_CACHE_TABLE "mqtt_cache"
#define TX_BUFF_TO_CACHE_USAGE (75.0)
#define CACHE_TO_TX_BUFF_USAGE (65.0)
#define CACHE_FILE_MEMORY_SIZE_MAX (40 * 1024 * 1024)

static mqtt_ringbuffer_t mqtt_txbuf;
static pthread_mutex_t mqtt_txbuf_mutex;
static mqtt_ringbuffer_t mqtt_rxbuf;
static pthread_mutex_t mqtt_rxbuf_mutex;

struct mqtt_t mqtt;
static void *mqtt_cache_handle = NULL;

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
        strncpy(e.szpayload, (const char *)message->payload, message->payloadlen);
        log_dbg("%s, payload:%s", __func__, e.szpayload);
        MQTTClient_freeMessage(&message);
        MQTTClient_free(topicName);
        mqtt_lock_rxbuf();
        mqtt_queue_rxbuf(e);
        mqtt_unlock_rxbuf();
    }
}

// static int mqtt_msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
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
//     mqtt_ringbuffer_element_t e;

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

//             mqtt_lock_txbuf();
//             mqtt_ringbuffer_queue(mqtt_rb, e);
//             mqtt_unlock_txbuf();

//         }
//     }
//     #endif

//     MQTTClient_freeMessage(&message);
//     MQTTClient_free(topicName);

//     return 0;
// }

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
int mqtt_connect(void)
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
        log_dbg("%s, MQTTClient_connect fail, msg:%s  %s %s", __func__, MQTTClient_strerror(rc), mqtt->szclientid, mqtt->szusername);
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

int mqtt_sub(const char *topic, int qos)
{
    struct mqtt_t *dev = &mqtt;
    int rc = 0;
    MQTTResponse response = MQTTResponse_initializer;
    response = MQTTClient_subscribe5(dev->cli, topic, qos, NULL, NULL);
    if (response.reasonCode != MQTTCLIENT_SUCCESS && response.reasonCode != qos)
    {
        log_dbg("%s, MQTTClient_subscribe fail, rc: %d msg:%s", __func__, response.reasonCode, MQTTClient_strerror(response.reasonCode));
        rc = -1;
    }
    MQTTResponse_free(response);
    return rc;
}

int mqtt_get_state()
{
    return mqtt.sm.state;
}

int mqtt_get_tz()
{
    return mqtt.timezone;
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

int mqtt_get_cmd()
{
    return mqtt.cmd;
}

void mqtt_reset_cmd()
{
    mqtt.cmd = CMD_SM_DONE;
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

int mqtt_pub(char *sztopic, char *szpayload)
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
    MQTTResponse response = MQTTResponse_initializer;
    response = MQTTClient_publishMessage5(dev->cli, sztopic, &msg, &dev->token);
    if (response.reasonCode != MQTTCLIENT_SUCCESS)
    {
        log_dbg("%s, Failed to publish message: topic %s, payload %s, error msg : %s\n", __func__, sztopic, szpayload, MQTTClient_strerror(response.reasonCode));
        dev->pub_failed++;
        ret = -2;
        goto leave;
    }
    // printf("Waiting for up to %d seconds for publication of %s\n"
    //         "on topic %s for client with ClientID: %s\n",
    //         (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
    rc = MQTTClient_waitForCompletion(dev->cli, dev->token, 100000L);
    if (rc != MQTTCLIENT_SUCCESS)
    {
        log_dbg("%s, MQTTClient_waitForCompletion Failed, topic %s, payload %s, error msg : %s\n", __func__, sztopic, szpayload, MQTTClient_strerror(rc));
        dev->pub_failed++;
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

int mqtt_send_sm_cmd(int cmd)
{
    mqtt.cmd = cmd;
    log_dbg("%s, cmd:%d", __func__, cmd);
}

static void mqtt_deal_with_cache(void)
{
    struct mqtt_t *dev = &mqtt;
    double *txbuf_usage = &dev->txbuf_usage;
    mqtt_ringbuffer_element_t e;
    int rc = 0;
    int cache_item_nb = 0;
    int cache_item_idx = 0;
    static int count = 0;

    if (mqtt_cache_handle == NULL)
    {
        return;
    }

    if (*txbuf_usage > TX_BUFF_TO_CACHE_USAGE)
    {

        // get file memory size
        long cache_memroy_size = 0;
        mqtt_cache_get_memory_size(mqtt_cache_handle, &cache_memroy_size);
        if (cache_memroy_size > CACHE_FILE_MEMORY_SIZE_MAX)
        {
            return;
        }

        mqtt_lock_txbuf();
        rc = mqtt_peek_txbuf(&e, 0);
        mqtt_unlock_txbuf();
        if (rc == 1)
        {
            // write to cache
            if (mqtt_cache_write_one_payload(mqtt_cache_handle, MQTT_CACHE_TABLE, e) == 0)
            { // if success,delet from buf
                mqtt_lock_txbuf();
                mqtt_dequeue_txbuf(&e);
                mqtt_unlock_txbuf();
            }
        }
    }
    else if (*txbuf_usage < CACHE_TO_TX_BUFF_USAGE)
    {
        rc = mqtt_cache_get_payload_nb(mqtt_cache_handle, MQTT_CACHE_TABLE, &cache_item_nb);
        if (cache_item_nb > 0 && rc == 0)
        { // have payload in cache
            // read one payload
            e.sztopic[0] = 0;
            e.szpayload[0] = 0;
            mqtt_cache_read_one_payload(mqtt_cache_handle, MQTT_CACHE_TABLE, &e, &cache_item_idx);
            e.cmd = CMD_MQTT_SENDKV;
            // add to txbuf
            mqtt_lock_txbuf();
            mqtt_queue_txbuf(e);
            mqtt_unlock_txbuf();
            // delete from cache
            mqtt_cache_delete_one_payload(mqtt_cache_handle, MQTT_CACHE_TABLE, cache_item_idx);
            count++;
            if (count > 500)
            {
                count = 0;
                // free memory
                mqtt_cache_free_memory(mqtt_cache_handle);
            }
        } // else have no data in cache
    }
    else
    {
        // do nothing
    }
}

static void mqtt_db_cache_thrd_main(void *param)
{
    log_dbg("%s, ++", __func__);
    // open or cread cache table
    mqtt_cache_open(MQTT_CACHE_DATA_BASE, MQTT_CACHE_TABLE, &mqtt_cache_handle);
    while (1)
    {
        mqtt_deal_with_cache();
        usleep(10000); /* 10ms */
    }
    log_dbg("%s, --", __func__);
}

static void *mqtt_thrd_main(void *param)
{
    struct mqtt_t *dev = &mqtt;
    pthread_t xthrd;

    log_dbg("%s, ++", __func__);

    /* reset pub timings */
    dev->pub_totalcnt = 0.0;
    dev->pub_totaltime = 0.0;
    dev->pub_ave = 0.0;
    dev->pub_max = 0.0;
    mqtt_init_txbuf();
    mqtt_init_rxbuf();
    pthread_mutex_init(&mqtt_txbuf_mutex, NULL);
    pthread_mutex_init(&mqtt_rxbuf_mutex, NULL);

    mqtt_sm_init();

    // if(pthread_create(&xthrd,NULL, mqtt_db_cache_thrd_main, NULL)!=0){
    //     log_dbg( "%s, create mqtt_db_cache_thrd_main fail", __func__);
    // }

    // sleep(30); // NOTE !!!
    while (1)
    {
        mqtt_sm();
        // mqtt_deal_with_cache();
        usleep(10000); /* 10ms */
    }

    log_dbg("%s, --", __func__);
    return NULL;
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
        else if (strcmp("timezone", columnname[i]) == 0)
        {
            dev->timezone = atoi(columnvalue[i]);
        }
    }

    pcbparam->ret = 0;
    log_dbg("%s, mqtt config : %s, --,ret:%d", __func__, dev->szservip, pcbparam->ret);
    log_dbg("%s, --,ret:%d", __func__, pcbparam->ret);
    return 0;
}

int mqtt_init()
{
    pthread_t xthrd;
    int rc = 0;
    int ret = 0;
    struct mqtt_t *dev = &mdl.mqtt;
    char *errmsg = NULL;
    char sql[1024];
    struct dbcbparam_t cbparam;
    sqlite3 *db = NULL;

    log_dbg("%s, ++", __func__);

    plt_lock_projdb();
    db = plt_get_projdb();
    sprintf(sql, "select * from mqtt");
    cbparam.nrow = 0;
    rc = sqlite3_exec(db, sql, mqtt_dbcb_0, (void *)&cbparam, &errmsg);
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
        // sprintf(dev->szclientid, "ctn-%d-%s", ctn[1].idx_in_ess, ctn[1].szprojId);
        if (pthread_create(&xthrd, NULL, mqtt_thrd_main, NULL) != 0)
        {
            log_dbg("%s, create mqtt thrd main fail", __func__);
            ret = -1;
        }
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

char *mqtt_get_state_str(void)
{
    return mqtt.sm.szstate;
}

int mqtt_get_stp(void)
{
    return mqtt.sm.step;
}

char *mqtt_get_err_str(void)
{
    return mqtt.sm.szerrcode;
}

int mqtt_get_tick(void)
{
    return mqtt.sm.tick;
}

double mqtt_get_timing_ave(void)
{
    return mqtt.sm.timing_ave;
}

double mqtt_get_timing_cur(void)
{
    return mqtt.sm.timing_cur;
}

double mqtt_get_timing_max(void)
{
    return mqtt.sm.timing_max;
}

int mqtt_get_enable(void)
{
    return mqtt.enable;
}

char *mqtt_get_servip_str(void)
{
    return mqtt.szservip;
}

int mqtt_get_servport(void)
{
    return mqtt.servport;
}

char *mqtt_get_client_id(void)
{
    return mqtt.szclientid;
}

double mqtt_get_txbuf_usage(void)
{
    return mqtt.txbuf_usage;
}

char *mqtt_get_access_token(void)
{
    return mqtt.szaccesstoken;
}

int mqtt_get_tool_data(char *buf)
{
    struct mqtt_t *dev = &mqtt;
    struct statemachine_t *sm = &dev->sm;

    char buf_temp[8192];

    sprintf(buf, " MQTT ");

    sm_get_summary(sm, buf_temp, sizeof(buf_temp));
    strcat(buf, buf_temp);

    sprintf(buf_temp, "   en:%d tz:%d servip:%s servport:%d clientid:%s accesstoken:%s txbufusage:%.1f rxbufusage:%.1f\n",
            dev->enable, dev->timezone, dev->szservip, dev->servport, dev->szclientid, dev->szaccesstoken, dev->txbuf_usage, dev->rxbuf_usage);
    strcat(buf, buf_temp);

    return 0;
}

int mqtt_set_dbg(int val)
{
    struct mqtt_t *dev = &mqtt;
    dev->dbg = val;
    return 0;
}
