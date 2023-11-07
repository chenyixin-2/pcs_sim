#include "appl.h"
#include "plt.h"
#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <syslog.h>
#include <unistd.h>

#define shm_lock() pthread_mutex_lock(&SHM->mutex)
#define shm_unlock() pthread_mutex_unlock(&SHM->mutex)

static const char *pstr_sem_name_ping = "sta_sem_ping";
static const char *pstr_sem_name_pong = "sta_sem_pong";

static int bmain_run = 1;
static int bmenu_sta_run = 1;
static int bmenu_ctn_run = 1;
static int bmenu_chan_run = 1;
static int bmenu_misc_run = 1;
static int bmenu_meter_run = 1;
static int bmenu_mqtt_run = 1;
static int bmenu_cloud_run = 1;
static int bmenu_ems_run = 1;

typedef struct
{
    int bHide;
    const char *szInfo;
    void (*func)(void);
} menu_t;

static struct shm_t *SHM = NULL;
static struct shm_t SHMBUF;
static sem_t *semPing;
static sem_t *semPong;

// 0 : confirm
//-1 : not confirem
static int getConfirm(void)
{
    int ret = 0;
    char c;
    printf("Confirm?y/n:");
    getchar();
    c = getchar();

    return (c == 'y') ? (0) : (-1);
}

static int tool_send_shmcmd(int cmd, struct shmparam_t *param)
{
    int ret = 0;
    int rc;
    struct shm_t *shm = SHM;

    shm_lock();

    shm->cmd = cmd;
    if (param != NULL)
    {
        shm->param = *param;
    }
    rc = sem_post(semPing);
    if (rc < 0)
    {
        ret = -1;
        goto leave;
    }

    sem_wait(semPong);
    if (rc == 0)
    {
        memcpy((void *)&SHMBUF, (void *)SHM, sizeof(struct shm_t));
    }
    else
    {
        ret = -2;
    }
    while (sem_trywait(semPong) == 0)
        ;

leave:
    shm_unlock();
    return ret;
}

static int update_station(void)
{
    int ret = 0;

    if (tool_send_shmcmd(CMD_STA_FETCH, NULL) < 0)
    {
        ret = -1;
        goto leave;
    }
    else
    {
        MDL = *(struct mdl_t *)SHMBUF.buf;
    }
leave:
    if (ret < 0)
    {
        printf("%s, ret:%d\n", __func__, ret);
    }
    return ret;
}

static void print_chan()
{
    int i;
    struct mdl_t *sta = &MDL;
    struct chan_t *chan;
    int *channbr = &MDL.channbr;

    printf(REVERSE "CHAN" NONE "  nbr:%d\n", *channbr);
    for (i = 1; i <= *channbr; i++)
    {
        chan = &(sta->chan[i]);
        printf("  [%d]info:%s mode:%s dev:%s port:%d servip:%s servport:%d baud:%d dbg:%d en:%d rstcnt:%.0f\n",
               i, chan->szinfo, chan->szmode, chan->szdev, chan->port, chan->servip, chan->servport,
               chan->baud, chan->dbg, chan->en, chan->rstcnt);
    }
}

static void print_mqtt()
{
    struct mqtt_t *dev = &MDL.mqtt;
    printf(REVERSE "MQTT" NONE "  \n");
    printf("  stat:" YELLOW "%s" NONE " err:%s stp:%d servip:%s servport:%d clientid:%s username:%s passwd:%s\n",
           dev->sm.szstate, dev->sm.szerrcode, dev->sm.step, dev->szservip, dev->servport, dev->szclientid, dev->szusername, dev->szpasswd);
    printf("txbufusage:%.1f rxbufusage:%.1f sm tick:%03d ave:%.0f max:%.0fms pub_tot:%.0f ave:%.0f max:%.0f\n",
           dev->txbuf_usage, dev->rxbuf_usage, dev->sm.tick, dev->sm.timing_ave, dev->sm.timing_max,
           dev->pub_totalcnt, dev->pub_ave, dev->pub_max);
}

static void print_tbmqtt()
{
    struct tbmqtt_t *dev = &MDL.tbmqtt;
    printf(REVERSE "TB MQTT" NONE "  \n");
    printf("  stat:" YELLOW "%s" NONE " err:%s stp:%d servip:%s servport:%d clientid:%s accesstoken:%s \n",
           dev->sm.szstate, dev->sm.szerrcode, dev->sm.step, dev->szservip, dev->servport, dev->szclientid, dev->szaccesstoken);
    printf("txbufusage:%.1f rxbufusage:%.1f sm tick:%03d ave:%.0f max:%.0fms pub_total:%.0f ave:%.0f max:%.0f\n",
           dev->txbuf_usage, dev->rxbuf_usage, dev->sm.tick, dev->sm.timing_ave, dev->sm.timing_max,
           dev->pubTotalCnt, dev->pubAvg, dev->pubMax);
}

static void print_chan_bytab()
{
    int i;
    struct mdl_t *sta = &MDL;
    struct chan_t *chan;
    int *channbr = &MDL.channbr;

    //
    // CHAN  moved to print_chan
    //
    //  head
    printf(REVERSE "CHAN" NONE "  %s\n", sta->sztime);
    printf("%-4s%-8s%-14s%-14s%-9s\
%-8s%-10s%-4s\n",
           "idx", "info", "mode", "servip", "servport",
           "baud", "port", "Dbg");
    // main status
    for (i = 1; i <= *channbr; i++)
    {
        chan = &(sta->chan[i]);
        printf("%-4d%-8s%-14s%-14s%-9d\
%-8d%-10s%-4d\n",
               i, chan->szinfo, chan->szmode, chan->servip, chan->servport,
               chan->baud, chan->szdev, chan->dbg);
    }
}

static void print_sta(void)
{
    struct mdl_t *dev = &MDL;

    printf(REVERSE "STA" NONE " prj_id:%s sw_ver:%d.%d.%d time:%s time_zone:%d\n",
           MDL.szprojId, MDL.version[0], MDL.version[1], MDL.version[2], dev->sztime, dev->time_zone);
    printf(" stat:" YELLOW "%s" NONE " err:%s stp:%d info:%s ap:" L_GREEN "%d" NONE " aps:%d soc:" L_GREEN "%.1f" NONE " d|c:%d|%d norm_cap:%d pow:%d\n",
           dev->sm.szstate, dev->sm.szerrcode, dev->sm.step,
           dev->szinfo, dev->pow.active_p, dev->pow.active_p_set, dev->pow.soc,
           dev->pow.bdhgable, dev->pow.bchgable,
           dev->pow.norm_cap, dev->pow.norm_pow);

    printf(" Cloud upload_en:%d sys_intv[0]:%d [1]:%d [2]:%d\n",
           MDL.cloud.upload_enable, MDL.cloud.sys_intv[0], MDL.cloud.sys_intv[1], MDL.cloud.sys_intv[2]);
    printf(" MAC cpu_occupy:%.1f mem:%.1f disk:%.1f\n",
           MDL.mac.cpu_occupy, MDL.mac.mem_occupy, MDL.mac.disk_occupy);

    print_chan();
    print_mqtt();
    print_tbmqtt();
}

static void mfunc_exit(void)
{
    bmain_run = 0;
}

static void mfunc_print_sta(void)
{
    while (1)
    {
        update_station();
        print_sta();
        sleep(1);
        system("clear");
    }
}

static void mfunc_chan_return(void)
{
    bmenu_chan_run = 0;
}

static void mfunc_chan_print(void)
{
    while (1)
    {
        update_station();
        print_chan_bytab();

        sleep(1);
        system("clear");
    }
}

static void mfunc_chan_set_dbg(void)
{
    int idx = 1;
    struct shmparam_t param;
    int i;
    int start;
    int nb;
    int enable;
    printf("start_idx nb dbg\n");
    printf("dbg: 0-disable 1-enable\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &enable);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = enable;
        if (tool_send_shmcmd(CMD_CHAN_SET_DBG, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_chan_set_en(void)
{
    int idx = 1;
    struct shmparam_t param;
    int i;
    int start;
    int nb;
    int enable;
    printf("start_idx nb en\n");
    printf("en: 0-disable 1-enable\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &enable);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = enable;
        if (tool_send_shmcmd(CMD_CHAN_SET_EN, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_chan_reset(void)
{
    int idx = 1;
    struct shmparam_t param;
    int i;
    int start;
    int nb;
    int enable;
    printf("start_idx nb\n");
    printf(">");
    scanf("%d %d", &start, &nb);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        if (tool_send_shmcmd(CMD_CHAN_RESET, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static menu_t menu_chan[] = {
    {0, "return", mfunc_chan_return},
    {0, "print", mfunc_chan_print},
    {0, "set dbg", mfunc_chan_set_dbg},
    {0, "set en", mfunc_chan_set_en},
    {0, "reset", mfunc_chan_reset},
};
static void mfunc_chan(void)
{
    int i;
    int iinput;
    bmenu_chan_run = 1;
    while (bmenu_chan_run)
    {
        printf("\n\n-----------------\n CHAN MENU:\n");
        for (i = 0; i < sizeof(menu_chan) / sizeof(menu_t); i++)
        {
            if (menu_chan[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_chan[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_chan) / sizeof(menu_t))
        {
            if (menu_chan[iinput].bHide != 1)
            {
                menu_chan[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static void mfunc_meter_return(void)
{
    bmenu_meter_run = 0;
}

static void mfunc_meter_print(void)
{
    while (1)
    {
        update_station();
        // print_chan_bytab();
        sleep(1);
        system("clear");
    }
}

static void mfunc_meter_set_com_active_e(void)
{
    int idx = 1;
    int cmd;
    struct shmparam_t param;
    int i;
    // int bidx;
    int start;
    int nb;
    double dval;
    printf("start_idx nb com_active_e\n");
    printf("com_active_e:double\n");
    printf(">");
    scanf("%d %d %lf", &start, &nb, &dval);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.dval = dval;
        if (tool_send_shmcmd(CMD_METER_SET_COMACTIVE, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_meter_set_com_active_p(void)
{
    int idx = 1;
    int cmd;
    struct shmparam_t param;
    int i;
    // int bidx;
    int start;
    int nb;
    double dval;
    printf("start_idx nb com_active_p\n");
    printf("com_active_p:double\n");
    printf(">");
    scanf("%d %d %lf", &start, &nb, &dval);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.dval = dval;
        if (tool_send_shmcmd(CMD_METER_SET_COMACTIVP, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_meter_set_pt_mdl(void)
{
    int idx = 1;
    int cmd;
    struct shmparam_t param;
    int i;
    // int bidx;
    int start;
    int nb;
    int val;
    printf("start_idx nb PT\n");
    printf("PT:int\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &val);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = val;
        if (tool_send_shmcmd(CMD_METER_SET_PT_MDL, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_meter_set_ct_mdl(void)
{
    int idx = 1;
    int cmd;
    struct shmparam_t param;
    int i;
    // int bidx;
    int start;
    int nb;
    int val;
    printf("start_idx nb CT\n");
    printf("CT:int\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &val);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = val;
        if (tool_send_shmcmd(CMD_METER_SET_CT_MDL, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_meter_sendcmd(void)
{
    int cmd;
    struct shmparam_t param;
    int start;
    int nb;
    int i;

    printf("meter cmd list:\n");
    printf("%d : launch\n", CMD_SM_LAUNCH);
    printf("%d : ready\n", CMD_SM_READY);
    printf("%d : ack\n", CMD_SM_ACK);
    printf("%d : nack\n", CMD_SM_NACK);
    printf("start_idx nb cmd\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &cmd);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = cmd;
        if (tool_send_shmcmd(CMD_METER_SENDCMD, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_meter_set_pt(void)
{
    int idx = 1;
    int cmd;
    struct shmparam_t param;
    int i;
    // int bidx;
    int start;
    int nb;
    int val;
    printf("start_idx nb PT\n");
    printf("PT:int\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &val);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = val;
        if (tool_send_shmcmd(CMD_METER_SET_PT, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_meter_set_ct(void)
{
    int idx = 1;
    int cmd;
    struct shmparam_t param;
    int i;
    // int bidx;
    int start;
    int nb;
    int val;
    printf("start_idx nb CT\n");
    printf("CT:int\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &val);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = val;
        if (tool_send_shmcmd(CMD_METER_SET_CT, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static menu_t menu_meter[] = {
    {0, "return", mfunc_meter_return},
    {0, "send cmd", mfunc_meter_sendcmd},
    {0, "set PT", mfunc_meter_set_pt},
    {0, "set CT", mfunc_meter_set_ct},
};
static void mfunc_meter(void)
{
    int i;
    int iinput;
    bmenu_meter_run = 1;
    while (bmenu_meter_run)
    {
        printf("\n\n-----------------\n METER MENU:\n");
        for (i = 0; i < sizeof(menu_meter) / sizeof(menu_t); i++)
        {
            if (menu_meter[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_meter[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_meter) / sizeof(menu_t))
        {
            if (menu_meter[iinput].bHide != 1)
            {
                menu_meter[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static void mfunc_mqtt_return(void)
{
    bmenu_mqtt_run = 0;
}

static void mfunc_mqtt_sendcmd(void)
{
    int cmd;
    struct shmparam_t param;

    printf("mqtt cmd list:\n");
    printf("%d : offline\n", CMD_SM_OFFLINE);
    printf("%d : stdby\n", CMD_SM_READY);
    printf("%d : ack\n", CMD_SM_ACK);
    printf("%d : nack\n", CMD_SM_NACK);
    printf("cmd\n");
    printf(">");
    scanf("%d", &cmd);

    param.val = cmd;
    if (tool_send_shmcmd(CMD_MQTT_SENDCMD, &param) < 0)
    {
        printf("tool send cmd fail\n");
    }
    else
    {
        printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
    }
}

static menu_t menu_mqtt[] = {
    {0, "return", mfunc_mqtt_return},
    {0, "send cmd", mfunc_mqtt_sendcmd},
};
static void mfunc_mqtt(void)
{
    int i;
    int iinput;
    bmenu_mqtt_run = 1;
    while (bmenu_mqtt_run)
    {
        printf("\n\n-----------------\n MQTT MENU:\n");
        for (i = 0; i < sizeof(menu_mqtt) / sizeof(menu_t); i++)
        {
            if (menu_mqtt[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_mqtt[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_mqtt) / sizeof(menu_t))
        {
            if (menu_mqtt[iinput].bHide != 1)
            {
                menu_mqtt[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static void mfunc_misc_return(void)
{
    bmenu_misc_run = 0;
}

static void mfunc_misc_lockshm(void)
{
    printf("try to lock...");
    shm_lock();
    printf("done\n");
}

static void mfunc_misc_unlockshm(void)
{
    printf("try to unlock...");
    shm_unlock();
    printf("done\n");
}

static void mfunc_misc_post_semping(void)
{
    int rc = 0;
    printf("try to post semPing...");
    rc = sem_post(semPing);
    printf("rc:%d\n", rc);
}

static void mfunc_misc_wait_sempong(void)
{
    int rc = 0;
    printf("try to wait semPong...");
    rc = sem_wait(semPong);
    printf("rc:%d\n", rc);
}

static void mfunc_misc_printmac(void)
{
    int i;
    struct timeval tv;
    struct timezone tz;
    time_t timep;
    struct tm *tsp;

    double starttime;
    double endtime;
    double exetime;
    float fval;
    struct cpu_info_t cpu_info;

    printf("char:%d,short:%d,int:%d,long:%d,long long:%d,float:%d,double:%d,station_t:%d\n",
           sizeof(char), sizeof(short), sizeof(int), sizeof(long),
           sizeof(long long), sizeof(float), sizeof(double), sizeof(struct mdl_t));

    /* cal gettimeofday */
    gettimeofday(&tv, &tz);
    starttime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    for (i = 0; i < 100; i++)
    {
        misc_gettimeofday();
    }
    gettimeofday(&tv, &tz);
    endtime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    exetime = endtime - starttime;
    printf("gettimeofday exe time:%.3fms\n", exetime / 100.0);

    /* cal time and localtime */
    gettimeofday(&tv, &tz);
    starttime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    for (i = 0; i < 100; i++)
    {
        time(&timep);
        // tsp = gmtime(&timep);
        tsp = localtime(&timep);
    }
    gettimeofday(&tv, &tz);
    endtime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    exetime = endtime - starttime;
    printf("time&localtime exe time:%.3fms\n", exetime / 100.0);

    /* get cpu occupy */
    printf("getting cpu occupy, wait 5 seconds\n");
    gettimeofday(&tv, &tz);
    starttime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    for (i = 0; i < 100; i++)
    {
        mac_get_cpu_info(&cpu_info);
    }
    gettimeofday(&tv, &tz);
    endtime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    exetime = endtime - starttime;
    printf("mac_get_cpu_info exe time:%.3fms val:%.1f\n", exetime / 100.0, mac_get_cpu_occupy());

    /* get mem occupy */
    gettimeofday(&tv, &tz);
    starttime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    for (i = 0; i < 100; i++)
    {
        fval = mac_get_mem_occupy();
    }
    gettimeofday(&tv, &tz);
    endtime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    exetime = endtime - starttime;
    printf("mac_get_mem_occupy exe time:%.3fms val:%.1f\n", exetime / 100.0, mac_get_mem_occupy());

    /* get disk occupy */
    gettimeofday(&tv, &tz);
    starttime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    for (i = 0; i < 100; i++)
    {
        fval = mac_get_disk_occupy();
    }
    gettimeofday(&tv, &tz);
    endtime = (double)tv.tv_sec * 1000 + (double)tv.tv_usec / 1000;
    exetime = endtime - starttime;
    printf("mac_get_disk_occupy exe time:%.3fms val:%.1f\n", exetime / 100.0, mac_get_disk_occupy());
}

static void mfunc_misc_cjson_ex()
{
    cJSON *cjson_test = NULL;
    cJSON *cjson_address = NULL;
    cJSON *cjson_skill = NULL;
    char *str = NULL;

    /* 创建一个JSON数据对象(链表头结点) */
    cjson_test = cJSON_CreateObject();
    /* 添加一条字符串类型的JSON数据(添加一个链表节点) */
    cJSON_AddStringToObject(cjson_test, "name", "mculover666");
    /* 添加一条整数类型的JSON数据(添加一个链表节点) */
    cJSON_AddNumberToObject(cjson_test, "age", 22);
    /* 添加一条浮点类型的JSON数据(添加一个链表节点) */
    cJSON_AddNumberToObject(cjson_test, "weight", 55.5);
    /* 添加一个嵌套的JSON数据（添加一个链表节点） */
    cjson_address = cJSON_CreateObject();
    cJSON_AddStringToObject(cjson_address, "country", "China");
    cJSON_AddNumberToObject(cjson_address, "zip-code", 111111);
    cJSON_AddItemToObject(cjson_test, "address", cjson_address);
    /* 添加一个数组类型的JSON数据(添加一个链表节点) */
    cjson_skill = cJSON_CreateArray();
    cJSON_AddItemToArray(cjson_skill, cJSON_CreateString("C"));
    cJSON_AddItemToArray(cjson_skill, cJSON_CreateString("Java"));
    cJSON_AddItemToArray(cjson_skill, cJSON_CreateString("Python"));
    cJSON_AddItemToObject(cjson_test, "skill", cjson_skill);
    /* 添加一个值为 False 的布尔类型的JSON数据(添加一个链表节点) */
    cJSON_AddFalseToObject(cjson_test, "student");
    /* 打印JSON对象(整条链表)的所有数据 */
    str = cJSON_Print(cjson_test);
    printf("%s\n", str);
    cJSON_Delete(cjson_test);
}

static void mfunc_misc_mem_leak_ex()
{
    cJSON *cjson_test = NULL;
    cJSON *cjson_address = NULL;
    cJSON *cjson_skill = NULL;
    char *str = NULL;

    while (1)
    {
        /* 创建一个JSON数据对象(链表头结点) */
        cjson_test = cJSON_CreateObject();
        /* 添加一条字符串类型的JSON数据(添加一个链表节点) */
        cJSON_AddStringToObject(cjson_test, "name", "mculover666");
        /* 添加一条整数类型的JSON数据(添加一个链表节点) */
        cJSON_AddNumberToObject(cjson_test, "age", 22);
        /* 添加一条浮点类型的JSON数据(添加一个链表节点) */
        cJSON_AddNumberToObject(cjson_test, "weight", 55.5);
        /* 添加一个嵌套的JSON数据（添加一个链表节点） */
        cjson_address = cJSON_CreateObject();
        cJSON_AddStringToObject(cjson_address, "country", "China");
        cJSON_AddNumberToObject(cjson_address, "zip-code", 111111);
        cJSON_AddItemToObject(cjson_test, "address", cjson_address);
        /* 添加一个数组类型的JSON数据(添加一个链表节点) */
        cjson_skill = cJSON_CreateArray();
        cJSON_AddItemToArray(cjson_skill, cJSON_CreateString("C"));
        cJSON_AddItemToArray(cjson_skill, cJSON_CreateString("Java"));
        cJSON_AddItemToArray(cjson_skill, cJSON_CreateString("Python"));
        cJSON_AddItemToObject(cjson_test, "skill", cjson_skill);
        /* 添加一个值为 False 的布尔类型的JSON数据(添加一个链表节点) */
        cJSON_AddFalseToObject(cjson_test, "student");
        /* 打印JSON对象(整条链表)的所有数据 */
        str = cJSON_Print(cjson_test);
        printf("%s\n", str);
        usleep(5000);
    }
    cJSON_Delete(cjson_test);
}

static void mfunc_misc_ringbuffer_ex()
{
    // test_ring_buffer_element_t e;
    // int i, rc;

    // test_ring_buffer_init(ptestrb);
    // printf("test ring buffer inited, queue used:%d, q_size:%d\n",
    //        test_ring_buffer_num_items(ptestrb),
    //        test_ring_buffer_size(ptestrb));

    // printf("peek from 0 to 2\n");
    // for (i = 0; i < 3; i++)
    // {
    //     rc = test_ring_buffer_peek(ptestrb, &e, i);
    //     printf(" index:%d rc:%d", i, rc);
    //     if (rc == 1)
    //     {
    //         printf(" val:%d\n", e.val);
    //     }
    //     else
    //     {
    //         printf("\n");
    //     }
    // }

    // printf("queue 3 values\n");
    // for (i = 0; i < 3; i++)
    // {
    //     e.val = 11 + i;
    //     test_ring_buffer_queue(ptestrb, e);
    //     printf(" val:%d\n", e.val);
    // }
    // printf("\n");

    // printf("peek from 0 to 2\n");
    // for (i = 0; i < 3; i++)
    // {
    //     rc = test_ring_buffer_peek(ptestrb, &e, i);
    //     printf(" index:%d rc:%d", i, rc);
    //     if (rc == 1)
    //     {
    //         printf(" val:%d\n", e.val);
    //     }
    //     else
    //     {
    //         printf("\n");
    //     }
    // }

    // test_ring_buffer_dequeue(ptestrb, &e);
    // printf("dequeue 1 value : %d\n", e.val);

    // printf("peek from 0 to 2\n");
    // for (i = 0; i < 3; i++)
    // {
    //     rc = test_ring_buffer_peek(ptestrb, &e, i);
    //     printf(" index:%d rc:%d", i, rc);
    //     if (rc == 1)
    //     {
    //         printf(" val:%d\n", e.val);
    //     }
    //     else
    //     {
    //         printf("\n");
    //     }
    // }

    // e.val = 14;
    // test_ring_buffer_queue(ptestrb, e);
    // printf("queue 1 val:%d\n", e.val);

    // printf("peek from 0 to 2\n");
    // for (i = 0; i < 3; i++)
    // {
    //     rc = test_ring_buffer_peek(ptestrb, &e, i);
    //     printf(" index:%d rc:%d", i, rc);
    //     if (rc == 1)
    //     {
    //         printf(" val:%d\n", e.val);
    //     }
    //     else
    //     {
    //         printf("\n");
    //     }
    // }
}

static void mfunc_misc_snap_del_test()
{
}

static menu_t menu_misc[] = {
    {0, "return", mfunc_misc_return},
    {0, "print mac", mfunc_misc_printmac},
    {0, "lock shm", mfunc_misc_lockshm},
    {0, "unlock shm", mfunc_misc_unlockshm},
    {0, "post semPing", mfunc_misc_post_semping},
    {0, "wait semPong", mfunc_misc_wait_sempong},
    {0, "cJSON example", mfunc_misc_cjson_ex},
    {0, "ringbuffer example", mfunc_misc_ringbuffer_ex},
    {0, "mem leak example", mfunc_misc_mem_leak_ex},
    {0, "snap del test", mfunc_misc_snap_del_test},
};

static void mfunc_misc(void)
{
    int iinput;
    int i;
    bmenu_misc_run = 1;
    while (bmenu_misc_run)
    {

        printf("\n\n-----------------\n misc menu:\n");
        for (i = 0; i < sizeof(menu_misc) / sizeof(menu_t); i++)
        {
            if (menu_misc[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_misc[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_misc) / sizeof(menu_t))
        {
            if (menu_misc[iinput].bHide != 1)
            {
                menu_misc[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static void mfunc_ctn_return(void)
{
    bmenu_ctn_run = 0;
}

static void mfunc_ctn_sendcmd(void)
{
    int cmd;
    struct shmparam_t param;
    int start;
    int nb;
    int i;

    printf("cmd list:\n");
    printf("%d : offline\n", CMD_SM_OFFLINE);
    printf("%d : stdby\n", CMD_SM_STDBY);
    printf("%d : stop\n", CMD_SM_STOP);
    printf("%d : ready\n", CMD_SM_READY);
    // printf("%d : chg\n",                 CMD_SM_CHG);
    printf("%d : ack\n", CMD_SM_ACK);
    printf("%d : nack\n", CMD_SM_NACK);
    printf("start_idx nb cmd\n");
    printf(">");
    scanf("%d %d %d", &start, &nb, &cmd);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = cmd;
        if (tool_send_shmcmd(CMD_CTN_SENDCMD, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_ctn_set_aps(void)
{
    double dval;
    struct shmparam_t param;
    int start;
    int nb;
    int i;

    printf("start_idx nb aps\n");
    printf("type:double unit:kW pos:dhg neg:chg 0:idle\n");
    printf(">");
    scanf("%d %d %lf", &start, &nb, &dval);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.dval = dval;
        if (tool_send_shmcmd(CMD_CTN_SET_ACTIVEPSET, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static menu_t menu_ctn[] = {
    {0, "return", mfunc_ctn_return},
    {0, "send cmd", mfunc_ctn_sendcmd}, //
    {0, "set aps", mfunc_ctn_set_aps},  //
};

static void mfunc_ctn(void)
{
    int i;
    int iinput;
    bmenu_ctn_run = 1;
    while (bmenu_ctn_run)
    {
        printf("\n\n-----------------\nCTN MENU:\n");
        for (i = 0; i < sizeof(menu_ctn) / sizeof(menu_t); i++)
        {
            if (menu_ctn[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_ctn[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_ctn) / sizeof(menu_t))
        {
            if (menu_ctn[iinput].bHide != 1)
            {
                menu_ctn[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static void mfunc_sta_return(void)
{
    bmenu_sta_run = 0;
}

static void mfunc_sta_sendcmd(void)
{
    int cmd;
    struct shmparam_t param;
    int start;
    int nb;
    int i;

    printf("cmd list:\n");
    printf("%d : offline\n", CMD_SM_OFFLINE);
    printf("%d : stdby\n", CMD_SM_STDBY);
    printf("%d : stop\n", CMD_SM_STOP);
    printf("%d : ready\n", CMD_SM_READY);
    printf("%d : ack\n", CMD_SM_ACK);
    printf("%d : nack\n", CMD_SM_NACK);
    printf("cmd\n");
    printf(">");
    scanf("%d", &cmd);

    param.val = cmd;
    if (tool_send_shmcmd(CMD_STA_SENDCMD, &param) < 0)
    {
        printf("tool send cmd fail\n");
    }
    else
    {
        printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
    }
}

static void mfunc_sta_send_stdby_cmd(void)
{
    int cmd = CMD_SM_STDBY;
    struct shmparam_t param;

    param.val = cmd;
    if (tool_send_shmcmd(CMD_STA_SENDCMD, &param) < 0)
    {
        printf("tool send cmd fail\n");
    }
    else
    {
        printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
    }
}

static void mfunc_sta_send_stop_cmd(void)
{
    int cmd = CMD_SM_STOP;
    struct shmparam_t param;

    param.val = cmd;
    if (tool_send_shmcmd(CMD_STA_SENDCMD, &param) < 0)
    {
        printf("tool send cmd fail\n");
    }
    else
    {
        printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
    }
}

static void mfunc_sta_send_ready_cmd(void)
{
    int cmd = CMD_SM_READY;
    struct shmparam_t param;

    param.val = cmd;
    if (tool_send_shmcmd(CMD_STA_SENDCMD, &param) < 0)
    {
        printf("tool send cmd fail\n");
    }
    else
    {
        printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
    }
}

static void mfunc_sta_set_active_p_set(void)
{
    int val;
    struct shmparam_t param;

    printf("aps \nint kW pos:dhg neg:chg 0:idle\n");
    printf(">");
    scanf("%d", &val);

    param.val = val;
    if (tool_send_shmcmd(CMD_STA_SET_ACTIVEPSET, &param) < 0)
    {
        printf("tool send cmd fail\n");
    }
    else
    {
        printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
    }
}

static menu_t menu_sta[] = {
    {0, "return", mfunc_sta_return},
    {0, "send cmd", mfunc_sta_sendcmd},         //
    {0, "set aps", mfunc_sta_set_active_p_set}, //
};

static void mfunc_sta(void)
{
    int i;
    int iinput;
    bmenu_sta_run = 1;
    while (bmenu_sta_run)
    {
        printf("\n\n-----------------\nSTA MENU:\n");
        for (i = 0; i < sizeof(menu_sta) / sizeof(menu_t); i++)
        {
            if (menu_sta[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_sta[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_sta) / sizeof(menu_t))
        {
            if (menu_sta[iinput].bHide != 1)
            {
                menu_sta[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static void mfunc_cloud_return(void)
{
    bmenu_cloud_run = 0;
}

static void mfunc_cloud_set_ess_sys_en_intval(void)
{
    int en, intval;
    struct shmparam_t param;
    int start = 1;
    int nb = 1;
    int i;

    printf("enable interval\n");
    printf("enable 0:disable 1:enable\n");
    printf("interval unit:seconds\n");
    printf(">");
    scanf("%d %d", &en, &intval);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = en;
        param.val2 = intval;
        if (tool_send_shmcmd(CMD_CLOUD_SET_ESSSYS_ENINTVAL, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static menu_t menu_cloud[] = {
    {0, "return", mfunc_cloud_return},
    {0, "set ess_sys enable and interval", mfunc_cloud_set_ess_sys_en_intval},
};

static void mfunc_cloud(void)
{
    int i;
    int iinput;
    bmenu_cloud_run = 1;
    while (bmenu_cloud_run)
    {
        printf("\n\n-----------------\nCLOUD MENU:\n");
        for (i = 0; i < sizeof(menu_cloud) / sizeof(menu_t); i++)
        {
            if (menu_cloud[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_cloud[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_cloud) / sizeof(menu_t))
        {
            if (menu_cloud[iinput].bHide != 1)
            {
                menu_cloud[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static void mfunc_ems_return(void)
{
    bmenu_ems_run = 0;
}

static void mfunc_ems_set_pow_regulate_high_low(void)
{
    int high, low;
    struct shmparam_t param;
    int start = 1;
    int nb = 1;
    int i;

    printf("high low\n");
    printf("double kW\n");
    printf(">");
    scanf("%d %d", &high, &low);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val_arr[0] = high;
        param.val_arr[1] = low;
        if (tool_send_shmcmd(CMD_EMS_SET_POWREG_HIGHLOW, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_ems_set_pcurvidx(void)
{
    int pidx;
    struct shmparam_t param;
    int start = 1;
    int nb = 1;
    int i;

    printf("pcurv_idx\n");
    printf("int [1 16]\n");
    printf(">");
    scanf("%d", &pidx);

    for (i = 0; i < nb; i++)
    {
        param.idx = start + i;
        param.val = pidx;
        if (tool_send_shmcmd(CMD_EMS_SET_PIDX, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_ems_reset_pcurv(void)
{
    int pidx;
    struct shmparam_t param;
    int start = 1;
    int nb = 1;
    int i;

    for (i = 0; i < nb; i++)
    {
        if (tool_send_shmcmd(CMD_EMS_RESET_PCURV, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_ems_print_pcurv(void)
{
}

static void mfunc_ems_edit_pcurv(void)
{
    struct shmparam_t param;
    int start = 1;
    int nb = 1;
    int i;
    int starthh, startnn;
    int endhh, endnn;
    int aps;

    printf("start_hour start_minute end_hour end_minute aps\n");
    printf(">");
    scanf("%d %d %d %d %d", &starthh, &startnn, &endhh, &endnn, &aps);

    for (i = 0; i < nb; i++)
    {
        param.val_arr[0] = starthh;
        param.val_arr[1] = startnn;
        param.val_arr[2] = endhh;
        param.val_arr[3] = endnn;
        param.val = aps;
        if (tool_send_shmcmd(CMD_EMS_EDITPCURV, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static void mfunc_ems_save_pcurv(void)
{
    int pidx;
    struct shmparam_t param;
    int start = 1;
    int nb = 1;
    int i;

    for (i = 0; i < nb; i++)
    {
        if (tool_send_shmcmd(CMD_EMS_SAVEPCURV, &param) < 0)
        {
            printf("tool send cmd fail\n");
        }
        else
        {
            printf("tool send cmd ok, rsp:%d\n", SHMBUF.rsp);
        }
    }
}

static menu_t menu_ems[] = {
    {0, "return", mfunc_ems_return},
    {0, "set pow regulate high and low", mfunc_ems_set_pow_regulate_high_low},
    {0, "set pcurv idx", mfunc_ems_set_pcurvidx},
    {0, "reset pcurv", mfunc_ems_reset_pcurv},
    {0, "print pcurv", mfunc_ems_print_pcurv},
    {0, "edit pcurv", mfunc_ems_edit_pcurv},
    {0, "save pcurv", mfunc_ems_save_pcurv},
};

static void mfunc_ems(void)
{
    int i;
    int iinput;
    bmenu_ems_run = 1;
    while (bmenu_ems_run)
    {
        printf("\n\n-----------------\nEMS MENU:\n");
        for (i = 0; i < sizeof(menu_ems) / sizeof(menu_t); i++)
        {
            if (menu_ems[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_ems[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_ems) / sizeof(menu_t))
        {
            if (menu_ems[iinput].bHide != 1)
            {
                menu_ems[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }
}

static menu_t menu_main[] = {
    /* 0 - 9 quick launch */
    {0, "exit", mfunc_exit},                             // 0
    {0, "print sta", mfunc_print_sta},                   // 1
    {0, "sta send stdby cmd", mfunc_sta_send_stdby_cmd}, // 2
    {0, "sta send stop cmd", mfunc_sta_send_stop_cmd},   // 3
    {0, "sta send ready cmd", mfunc_sta_send_ready_cmd}, // 4
    {0, "sta set aps", mfunc_sta_set_active_p_set},      // 5
    {1, "r", mfunc_exit},                                // 6
    {1, "r", mfunc_exit},                                // 7
    {1, "r", mfunc_exit},                                // 8
    {1, "r", mfunc_exit},                                // 9

    /* 10 - 19 sta */
    {0, "sta", mfunc_sta},
    {0, "ems", mfunc_ems},
    {0, "cloud", mfunc_cloud},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},

    /* 20 - 29 ctn */
    {0, "ctn", mfunc_ctn},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},

    /* 30 - 39 chan */
    {0, "chan", mfunc_chan}, // 16
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},

    /* 40 - 49 pcs */
    {1, "pcs", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},

    /* 50 - 59 pack */
    {1, "pack", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},

    /* 60 - 69 meter */
    {0, "meter", mfunc_meter},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},

    /* 70 - 79 mqtt*/
    {0, "mqtt", mfunc_mqtt},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},

    /* 80 - 89 */
    {0, "misc", mfunc_misc},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
    {1, "reserved", mfunc_exit},
};

int tool_main()
{
    int ret = 0;
    int i;
    int shm_id = -1;
    void *shared_memory = NULL;
    int iinput;

    shm_id = shmget((key_t)SHMID, 0, 0666);
    if (shm_id < 0)
    {
        ret = -1;
        goto leave;
    }
    else
    {
        shared_memory = shmat(shm_id, NULL, 0);
        if (shared_memory == NULL)
        {
            ret = -2;
            goto leave;
        }
        else
        {
            SHM = (struct shm_t *)shared_memory;
            semPing = sem_open(pstr_sem_name_ping, 0, 0644, 0);
            if (semPing == SEM_FAILED)
            {
                ret = -3;
                goto leave;
            }
            else
            {
                semPong = sem_open(pstr_sem_name_pong, 0, 0644, 0);
                if (semPong == SEM_FAILED)
                {
                    ret = -4;
                    goto leave;
                }
                else
                {
                    printf("****************\n");
                    printf("tool init success\n");
                    printf("****************\n");
                }
            }
        }
    }

    bmain_run = 1;
    while (bmain_run)
    {
        printf("\n\n-----------------\nMain MENU:\n");
        for (i = 0; i < sizeof(menu_main) / sizeof(menu_t); i++)
        {
            if (menu_main[i].bHide != 1)
            {
                printf("  %d->%s\n", i, menu_main[i].szInfo);
            }
        }
        printf(">");
        scanf("%d", &iinput);
        if (iinput < sizeof(menu_main) / sizeof(menu_t))
        {
            if (menu_main[iinput].bHide != 1)
            {
                menu_main[iinput].func();
            }
            else
            {
                printf("Illegal selection\n");
            }
        }
        else
        {
            printf("Illegal selection\n");
        }
    }

leave:
    printf("%s, --", __func__);
    return 0;
}
