#include "cloud.h"
#include "plt.h"

/* in seconds */
long long cloud_get_unixts()
{
    return (long long)time(NULL);
}

/* in ms */
long long cloud_get_timezonets()
{
    // return (cloud_get_unixts() + sta_get_timezone()*(long long)3600)*(long long)1000;
    return cloud_get_unixts() * (long long)1000;
}

int cloud_init(void)
{
    log_dbg("%s, ++++ ", __func__);
    int ret = 0;

    cloud_enable_upload();

    MDL.cloud.sys_timer[0] = 0;
    MDL.cloud.sys_intv[0] = 10; // 10 means 1s

    log_dbg("%s, ---- ret:%d", __func__, ret);
    return ret;
}

static void cloud_update()
{
    struct cloud_t *c = &MDL.cloud;
    mqtt_ringbuffer_element_t e;
    char ts_buf[128];
    char itm_buf[2048];
    int i;

    log_dbg("%s,", __func__);

    /* 1 seconds */
    if (c->sys_timer[0]++ >= c->sys_intv[0])
    {        
        c->sys_timer[0] = 0;
        e.cmd = CMD_MQTT_SENDKV;
        e.sztopic[0] = 0;
        e.szpayload[0] = 0;

        cJSON *o = cJSON_CreateObject();
        cJSON_AddNumberToObject(o, "uab", MDL.uab);
        cJSON_AddNumberToObject(o, "uca", MDL.uca);
        cJSON_AddNumberToObject(o, "ubc", MDL.ubc);
        cJSON_AddNumberToObject(o, "ia", MDL.ia);
        cJSON_AddNumberToObject(o, "ib", MDL.ib);
        cJSON_AddNumberToObject(o, "ic", MDL.ic);
        char *ostr = cJSON_PrintUnformatted(o);

        misc_get_datetimestr(ts_buf, sizeof(ts_buf));
        sprintf(e.szpayload, "{\"ts\":\"%s\",\"data\":%s}", ts_buf, ostr);
        sprintf(e.sztopic, "%s", MDL.szDevName);

        log_dbg("%s, topic:%s, payload:%s", __func__,e.sztopic, e.szpayload);

        mqtt_lock_txbuf();
        mqtt_queue_txbuf(e);
        mqtt_unlock_txbuf();

        cJSON_Delete(o);
        cJSON_free(ostr);
    }
}

int cloud_is_upload_enable()
{
    return MDL.cloud.upload_enable;
}

int cloud_enable_upload()
{
    MDL.cloud.upload_enable = 1;
    return 0;
}

int cloud_disable_upload()
{
    MDL.cloud.upload_enable = 0;
    return 0;
}

static void cloud_proc_recv()
{
    cJSON *root = NULL;
    cJSON *cmd = NULL;
    cJSON *param = NULL;
    int aps = 0;
    int rc;
    char strResp[128];

    mqtt_ringbuffer_element_t e;
    mqtt_lock_rxbuf();
    rc = mqtt_dequeue_rxbuf(&e);
    mqtt_unlock_rxbuf();
    if (rc != 1)
        return;

    root = cJSON_Parse(e.szpayload);
    if (!root)
    {
        log_dbg("%s, cJSON failed to parse payload null", __func__);
        return;
    }

    cmd = cJSON_GetObjectItem(root, "cmd");
    if (!cmd)
    {
        log_dbg("%s, cJSON failed to get cmd ", __func__);
        return;
    }

    // { "cmd":"set uab", "param":220.0 }    
    if (cJSON_IsString(cmd))
    {
        snprintf(strResp, sizeof(strResp), "%s get cmd:%s", __func__, cmd->valuestring);
        if (strcmp(cmd->valuestring, "set uab") == 0)
        {
            param = cJSON_GetObjectItem(root, "param");
            MDL.uab = param->valuedouble;
        }
    }
    else // { "set_ctn_cmd":"stdby", "param":220.0 }
    {
        cJSON *subCmd = cmd->child;
        if (NULL == subCmd)
        {
            snprintf(strResp, sizeof(strResp), "subcmd is invalid");
            log_dbg("%s, subcmd cJSON_Parse null", __func__);
            goto JSON_DONE;
        }

        snprintf(strResp, sizeof(strResp), "ctn cmd %s received", subCmd->string);
        log_dbg("%s, get cmd:%s:%s", __func__, subCmd->string, cmd->valuestring);
        if (strcmp(subCmd->string, "set_ctn_cmd") == 0)
        {
            if (strcmp(subCmd->valuestring, "stdby") == 0)
            {
            }
        }
        else if (strcmp(subCmd->string, "meter_send_cmd") == 0)
        {
            int index, value;
            sscanf(subCmd->valuestring, "%d:%d", &index, &value);
            if (strcmp(subCmd->valuestring, "stdby") == 0)
            {
            }
            else if (strcmp(subCmd->valuestring, "stop") == 0)
            {
            }
            else if (strcmp(subCmd->valuestring, "ready") == 0)
            {
            }
        }
        else
        {
            log_dbg("%s, unknown cmd : %s:%s", __func__, subCmd->string, subCmd->valuestring);
        }
        mqtt_ringbuffer_element_t eResponds;
        eResponds.cmd = CMD_MQTT_SENDKV;
        sprintf(eResponds.szpayload, "{\"type\":\"response\",\"timestamp\":%lld,\"data\":\"%s\"}", cloud_get_timezonets(), strResp);
        sprintf(eResponds.sztopic, "%s-ctl", MDL.szDevName);
        mqtt_lock_txbuf();
        mqtt_queue_txbuf(eResponds);
        mqtt_unlock_txbuf();
    }

JSON_DONE:
    cJSON_Delete(root);
}

void cloud_exe(void)
{
    cloud_proc_recv();
    cloud_update();
}