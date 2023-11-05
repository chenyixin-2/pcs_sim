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
    int ret = 0;

    cloud_enable_upload();

    mdl.cloud.sys_timer[0] = 0;
    mdl.cloud.sys_intv[0] = 10; // 10 means 1s
    mdl.cloud.sys_timer[1] = 0;
    mdl.cloud.sys_intv[1] = 60;
    mdl.cloud.sys_timer[2] = 0;
    mdl.cloud.sys_intv[2] = 300;

    mdl.cloud.meter_timer[0] = 0;
    mdl.cloud.meter_intv[0] = 5;

    log_dbg("%s, ret:%d", __func__, ret);
    return ret;
}

static void cloud_upload_sys()
{
    struct cloud_t *c = &mdl.cloud;
    mqtt_ringbuffer_element_t e;
    char buf[128];
    char itm_buf[2048];
    int i;

    /* 1 seconds */
    if (c->sys_timer[0]++ >= c->sys_intv[0]){
        c->sys_timer[0] = 0;
        e.cmd = CMD_MQTT_SENDKV;
        e.sztopic[0] = 0;
        e.szpayload[0] = 0;

        /* sys state and err */
        sprintf(itm_buf, "{\"uab\":%.1lf,\"type\":0, \"state\":\"%d\",\"err\":\"%s\",\"ap\":%d,\"aps\":%d,\"chgable\":%d,\"dhgable\":%d,\"soc\":%.1f}",
                mdl.uab, sta_get_state(), sta_get_err_str(), sta_get_ap(), sta_get_aps(), sta_get_chgable(), sta_get_dhgable(), sta_get_soc());
        misc_get_datetimestr(buf,sizeof(buf));
        sprintf(e.szpayload, "{\"ts\":\"%s\",\"data\":[%s]}", buf, itm_buf);
        sprintf(e.sztopic, "%s", mdl.szdev);
        // printf("topic:%s payload:%s\n", e.sztopic, e.szpayload);
        mqtt_lock_txbuf();
        mqtt_queue_txbuf(e);
        mqtt_unlock_txbuf();
    }
}

int cloud_is_upload_enable()
{
    return mdl.cloud.upload_enable;
}

int cloud_enable_upload()
{
    mdl.cloud.upload_enable = 1;
    return 0;
}

int cloud_disable_upload()
{
    mdl.cloud.upload_enable = 0;
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
    if (rc == 1)
    {
        root = cJSON_Parse(e.szpayload);
        if (!root)
        {
            log_dbg("%s, cJSON_Parse null", __func__);
        }
        else
        {
            cmd = cJSON_GetObjectItem(root, "cmd");
            if (!cmd)
            {
                log_dbg("%s, cJSON_GetObjectItem cmd fail", __func__);
            }
            else
            {
                if(cJSON_IsString(cmd))
                {
                    snprintf(strResp,sizeof(strResp),"%s get cmd:%s", __func__, cmd->valuestring);
                    if (strcmp(cmd->valuestring, "set uab") == 0)
                    {
                        param = cJSON_GetObjectItem(root, "param");
                        mdl.uab = param->valuedouble;
                    }
                }
                else
                {
                    cJSON *subCmd = cmd->child;
                    if(NULL == subCmd)
                    {
                        snprintf(strResp,sizeof(strResp),"subcmd is  invalide");
                        log_dbg("%s, subcmd cJSON_Parse null", __func__);
                        goto JSON_DONE;
                    }

                    snprintf(strResp,sizeof(strResp),"ctn cmd %S received",subCmd->string);
                    log_dbg("%s, get cmd:%s:%s", __func__, subCmd->string,cmd->valuestring);
                    if (strcmp(subCmd->string, "set_ctn_cmd") == 0)
                    {
                        if (strcmp(subCmd->valuestring, "stdby") == 0)
                        {
                            sta_send_cmd(CMD_SM_STDBY);
                        }
                        else if (strcmp(subCmd->valuestring, "stop") == 0)
                        {
                            sta_send_cmd(CMD_SM_STOP);
                        }
                        else if (strcmp(subCmd->valuestring, "ready") == 0)
                        {
                            sta_send_cmd(CMD_SM_READY);
                        }
                        else if (strcmp(subCmd->valuestring, "offgrid") == 0)
                        {
                            sta_send_cmd(CMD_SM_OFFGRID);
                        }
                        else if (strcmp(subCmd->valuestring, "idle") == 0)
                        {
                            sta_send_cmd(CMD_SM_IDLE);
                        }
                    }
                    else if(strcmp(subCmd->string, "set_sta_aps") == 0)
                    {
                        sta_set_aps(atoi(subCmd->valuestring));
                    }
                    else if(strcmp(subCmd->string, "ems_set_powreg_highlow") == 0)
                    {
                        int hight,low;
                        sscanf(subCmd->valuestring,"%d:%d",&hight,&low);
                    }
                    else if(strcmp(subCmd->string, "ctn_set_aps") == 0)
                    {
                        int index,aps;
                        sscanf(subCmd->valuestring,"%d:%d",&index,&aps);
                    }
                    else if(strcmp(subCmd->string, "chan_reset") == 0)
                    {
                        chan_reset(atoi(subCmd->valuestring));
                    }
                    else if(strcmp(subCmd->string, "meter_send_cmd") == 0)
                    {
                        int index,value;
                        sscanf(subCmd->valuestring,"%d:%d",&index,&value);
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
                    else if(strcmp(subCmd->string, "meter_set_pt") == 0)
                    {
                    }
                    else if(strcmp(subCmd->string, "meter_set_ct") == 0)
                    {
                    }
                     else if ( strcmp(subCmd->string, "set_pcurv_idx") == 0 )
                     {
                     }
                    else
                    {
                        log_dbg("%s, unknown cmd : %s:%s", __func__,subCmd->string, subCmd->valuestring);
                    }
                    //返回应答
                    mqtt_ringbuffer_element_t eResponds;
                    eResponds.cmd = CMD_MQTT_SENDKV;
                    sprintf(eResponds.szpayload,"{\"type\":\"response\",\"trace_id\":\"%s\"，\"timestamp\":%lld,\"data\":\"%s\"}"
                                                        , sta_get_prjid(), cloud_get_timezonets(), strResp);
                    sprintf(eResponds.sztopic,"%s", "control");
                    mqtt_lock_txbuf();
                    mqtt_queue_txbuf(eResponds);
                    mqtt_unlock_txbuf();
                }
                return;
            }

        JSON_DONE:
            cJSON_Delete(root);
        }
    }
}

void cloud_exe(void)
{
    cloud_proc_recv();
    cloud_upload_sys();
}