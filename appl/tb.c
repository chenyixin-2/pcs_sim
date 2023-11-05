#include "tb.h"
#include "plt.h"
struct tb_t tb;
/* in seconds */
static long long tb_get_unixts()
{
    return (long long)time(NULL);
}

/* in ms */
long long tb_get_ts()
{
    return (tb_get_unixts() + 0 * (long long)3600) * (long long)1000;
    // return tb_get_unixts()*1000;
}

static int tb_upload_meter()
{
    struct tb_t *c = &tb;
    tbmqtt_ringbuffer_element_t e;
    char buf[128];
    char itm_buf[2048];
    int i;

    if (c->meter_intv[0] > 0)
    {
        if (c->meter_timer[0]++ >= c->meter_intv[0])
        {
            c->meter_timer[0] = 0;
            e.cmd = CMD_MQTT_SENDKV;
            e.sztopic[0] = 0;
            e.szpayload[0] = 0;

            /* sw version */
            log_dbg("%s, data : %s", __func__, itm_buf);
            sprintf(e.szpayload, "{'ts':%lld,'values':{%s}}", tb_get_ts(), itm_buf); /* thingsboard */
            // sprintf(e.sztopic,"ems/data/rtEss/%s", sta_get_sn());
            strcpy(e.sztopic, "v1/devices/me/telemetry");

            tbmqtt_lock_txbuf();
            tbmqtt_queue_txbuf(e);
            tbmqtt_unlock_txbuf();
        }
    }

    return 0;
}

static int tb_send_rpc_response(char *request_topic, char *response_payload)
{
    char *p = NULL;
    char szreqid[256];
    char *pszreqid = szreqid;
    int reqid = 0;
    tbmqtt_ringbuffer_element_t e;

    p = strrchr(request_topic, '/');
    if (p != NULL)
    {
        p++;
        while (*p != 0)
        {
            *pszreqid++ = *p++;
        }
        *pszreqid = 0;
        reqid = atoi(szreqid);

        e.cmd = CMD_MQTT_SENDKV;
        e.sztopic[0] = 0;
        e.szpayload[0] = 0;
        strcpy(e.szpayload, response_payload);
        sprintf(e.sztopic, "v1/devices/me/rpc/response/%s", szreqid);

        log_dbg("%s, topic:%s, payload:%s", __func__, e.sztopic, e.szpayload);
        tbmqtt_lock_txbuf();
        tbmqtt_queue_txbuf(e);
        tbmqtt_unlock_txbuf();
    }

    return 0;
}

static int tb_update_attributes(char *payload)
{
    tbmqtt_ringbuffer_element_t e;

    e.cmd = CMD_MQTT_SENDKV;
    e.sztopic[0] = 0;
    e.szpayload[0] = 0;
    sprintf(e.szpayload, "{%s}", payload);
    strcpy(e.sztopic, "v1/devices/me/attributes");

    log_dbg("%s, topic:%s, payload:%s", __func__, e.sztopic, e.szpayload);
    tbmqtt_lock_txbuf();
    tbmqtt_queue_txbuf(e);
    tbmqtt_unlock_txbuf();

    return 0;
}

static void tb_upload_param()
{
    struct tb_t *dev = &tb;
    tbmqtt_ringbuffer_element_t e;
    char buf[128];
    char itm_buf[2048];
    int i;

    /* 5 seconds period */
    if (dev->param_en > 0)
    {
        dev->param_en = 0;
        e.cmd = CMD_MQTT_SENDKV;
        e.sztopic[0] = 0;
        e.szpayload[0] = 0;

        /* sw version */
        sprintf(itm_buf, "'sw_ver':'%s', 'norm_cap':%d,'norm_pow':%d",
                plt_get_sw_ver(), sta_get_norm_cap(), sta_get_norm_pow());
        sprintf(e.szpayload, "{'ts':%lld,'values':{%s}}", tb_get_ts(), itm_buf); /* thingsboard */
        // sprintf(e.sztopic,"ems/data/rtEss/%s", sta_get_sn());
        strcpy(e.sztopic, "v1/devices/me/telemetry");

        tbmqtt_lock_txbuf();
        tbmqtt_queue_txbuf(e);
        tbmqtt_unlock_txbuf();
    }
}

static void tb_upload_sys()
{
    struct tb_t *c = &tb;
    tbmqtt_ringbuffer_element_t e;
    char buf[128];
    char itm_buf[2048];
    int i;

    /* 10 seconds period */
    if (c->sys_intv[0] > 0)
    {
        if (c->sys_timer[0]++ >= c->sys_intv[0])
        {
            c->sys_timer[0] = 0;
            e.cmd = CMD_MQTT_SENDKV;
            e.sztopic[0] = 0;
            e.szpayload[0] = 0;

            // sys state and err
            itm_buf[0] = 0;
            sprintf(itm_buf, "'state':'%s','err':'%s','ap':%d",            
                    sta_get_state_str(),
                    sta_get_err_str(),
                    sta_get_ap());
            sprintf(e.szpayload, "{'ts':%lld,'values':{%s}}", tb_get_ts(), itm_buf);
            strcpy(e.sztopic, "v1/devices/me/telemetry");
            tbmqtt_lock_txbuf();
            tbmqtt_queue_txbuf(e);
            tbmqtt_unlock_txbuf();
        }
    }

    /* 60 seconds period */
    if (c->sys_intv[1] > 0)
    {
        if (c->sys_timer[1]++ >= c->sys_intv[1])
        {
            c->sys_timer[1] = 0;
            e.cmd = CMD_MQTT_SENDKV;
            e.sztopic[0] = 0;
            e.szpayload[0] = 0;

            itm_buf[0] = 0;
            /* cpu mem disk */
            sprintf(buf, "'soc':%.1f", sta_get_soc());
            strcat(itm_buf, buf);

            sprintf(e.szpayload, "{'ts':%lld,'values':{%s}}", tb_get_ts(), itm_buf);
            strcpy(e.sztopic, "v1/devices/me/telemetry");

            tbmqtt_lock_txbuf();
            tbmqtt_queue_txbuf(e);
            tbmqtt_unlock_txbuf();
        }
    }
}

#if 0
static void tb_upload_meter( void )
{
    struct tb_t* dev = &tb;
    tbmqtt_ringbuffer_element_t e;
    char buf[8192];
    char itm_buf[8192];
    int i;

    /* 60 seconds */
    if( dev->meter_intv[0] > 0){
        if( dev->meter_timer[0]++ >= dev->meter_intv[0] ){
            dev->meter_timer[0] = 0;

            // dtsd1352
            e.cmd = CMD_MQTT_SENDKV;
            e.sztopic[0] = 0;
            e.szpayload[0] = 0;

            memset(itm_buf,0,sizeof(itm_buf));
            meter_get_tbmqtt_data(itm_buf);

            sprintf(e.szpayload,"{'ts':%lld,'values':{%s}}", tb_get_ts(), itm_buf);

            //printf("%s,%s\n",__func__,e.szpayload);

            strcpy(e.sztopic,"v1/devices/me/telemetry");
            tbmqtt_lock_txbuf();
            tbmqtt_queue_txbuf(e);
            tbmqtt_unlock_txbuf();

        }
    }
}
#endif

int tb_init(void)
{
    log_dbg("%s, ++, ret:%d", __func__);

    struct tb_t *dev = &tb;
    int ret = 0;

    dev->upload_enable = 1;
    dev->param_en = 0;
    dev->sys_intv[0] = 10;
    dev->sys_intv[1] = 60;

    dev->meter_intv[0] = 5;

    dev->tb_lock_timer = 0;
    dev->tb_lock = 1;
    dev->tb_lock_intv = 15;

    log_dbg("%s, --, ret:%d", __func__, ret);
    return ret;
}

static int tmpaps_received = 0;
static int tmpaps;

static void tb_proc_recv()
{
    struct tb_t *dev = &tb;

    static int init = 0;
    if(init == 0)
    {
        init = 1;
        tb_update_attributes("'TbLock':true");
    }
    
    if ((dev->tb_lock == 0) && (dev->tb_lock_timer++ >= dev->tb_lock_intv))
    {
        dev->tb_lock_timer = 0;
        dev->tb_lock = 1;
        tb_update_attributes("'TbLock':true");
    }

    cJSON *root = NULL;
    cJSON *item = NULL;
    int aps = 0;
    int rc;
    tbmqtt_ringbuffer_element_t e;

    tbmqtt_lock_rxbuf();
    rc = tbmqtt_dequeue_rxbuf(&e);
    tbmqtt_unlock_rxbuf();
    if (rc == 1)
    {
        log_dbg("%s, get topic:%s", __func__, e.sztopic);
        log_dbg("%s, and payload:%s", __func__, e.szpayload);

        root = cJSON_Parse(e.szpayload);
        if (!root)
        {
            log_dbg("%s, cJSON_Parse null", __func__);
        }
        else
        {
            item = cJSON_GetObjectItem(root, "method");
            if (!item)
            {
                log_dbg("%s, cJSON_GetObjectItem method fail", __func__);
            }
            else
            {
                log_dbg("%s, get method:%s", __func__, item->valuestring);
                if (strcmp(item->valuestring, "set_lock") == 0)
                {
                    item = cJSON_GetObjectItem(root, "params");
                    if (!item)
                    {
                        log_dbg("%s, set_lock failed to get params ", __func__);
                    }
                    else
                    {
                        log_dbg("%s, set_lock, value :%s", __func__, item->valuestring);
                        if (strcmp(item->valuestring, "lock") == 0)
                        {
                            dev->tb_lock = 1;
                        }
                        else
                        {
                            dev->tb_lock = 0;
                        }
                    }
                }
                else if (strcmp(item->valuestring, "get_lock") == 0)
                {
                    sprintf(e.szpayload, "%d", dev->tb_lock);
                    tb_send_rpc_response(e.sztopic, e.szpayload);
                }
                else if (strcmp(item->valuestring, "get_aps") == 0)
                {
                    sprintf(e.szpayload, "%d", sta_get_aps());
                    tb_send_rpc_response(e.sztopic, e.szpayload);
                }
                else
                {
                    if (dev->tb_lock)
                    {
                        log_dbg("%s, thingsboard is locked, do nothing", __func__);
                    }
                    else
                    {
                        if (strcmp(item->valuestring, "stdby") == 0)
                        {
                            sta_send_cmd(CMD_SM_STDBY);
                        }
                        else if (strcmp(item->valuestring, "stop") == 0)
                        {
                            sta_send_cmd(CMD_SM_STOP);
                        }
                        else if (strcmp(item->valuestring, "ready") == 0)
                        {
                            sta_send_cmd(CMD_SM_READY);
                        }
                        else if (strcmp(item->valuestring, "offgrid") == 0)
                        {
                            sta_send_cmd(CMD_SM_OFFGRID);
                        }
                        else if (strcmp(item->valuestring, "active_aps") == 0)
                        {
                            if (tmpaps_received == 1)
                            {
                                sta_set_aps(tmpaps);
                                tmpaps_received = 0;
                            }
                        }
                        else if (strcmp(item->valuestring, "set_aps") == 0)
                        {
                            item = cJSON_GetObjectItem(root, "params");
                            if (!item)
                            {
                                log_dbg("%s, cJSON_GetObjectItem params fail", __func__);
                            }
                            else
                            {
                                // ess_set_aps(item->valueint);
                                log_dbg("%s, aps:%d", __func__, item->valueint);
                                tmpaps = item->valueint;
                                tmpaps_received = 1;
                            }
                        }
                        else
                        {
                            log_dbg("%s, unknown cmd : %s", __func__, item->valuestring);
                        }
                    }
                }
            }
            cJSON_Delete(root);
        }
    }
}

void tb_exe(void)
{
    tb_proc_recv();

    if (tb.upload_enable)
    {
        tb_upload_param();
        tb_upload_sys();
        tb_upload_meter();
    }
}

int tb_set_param_en(int val)
{
    struct tb_t *dev = &tb;
    dev->param_en = val;
}
