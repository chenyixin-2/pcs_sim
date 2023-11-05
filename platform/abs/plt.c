#include "plt.h"

struct mdl_t mdl;
static int cfg_db_init()
{
    int ret = 0;
    int rc = 0;
    char buf[128];

    sprintf(buf, "../cfg/sta.db");

    if (access(buf, 0) < 0)
    {
        ret = -1;
    }
    else
    {
        rc = sqlite3_open(buf, &mdl.cfg_db);
        if (rc == SQLITE_OK)
        {
            pthread_mutex_init(&mdl.cfg_db_mutex, NULL);
        }
        else
        {
            ret = -2;
        }
    }

    log_dbg("%s, ret:%d", __func__, ret);
    return ret;
}

void plt_lock_cfgdb()
{
    pthread_mutex_lock(&mdl.cfg_db_mutex);
}

void plt_unlock_cfgdb()
{
    pthread_mutex_unlock(&mdl.cfg_db_mutex);
}

sqlite3 *plt_get_cfgdb()
{
    return mdl.cfg_db;
}
static int project_db_init()
{
    int ret = 0;
    int rc = 0;
    char buf[128];

    sprintf(buf, "../cfg/project.db");

    if (access(buf, 0) < 0)
    {
        ret = -1;
    }
    else
    {
        rc = sqlite3_open(buf, &mdl.proj_db);
        if (rc == SQLITE_OK)
        {
            pthread_mutex_init(&mdl.proj_db_mutex, NULL);
        }
        else
        {
            ret = -2;
        }
    }

    log_dbg("%s, ret:%d", __func__, ret);
    return ret;
}

void plt_lock_projdb()
{
    pthread_mutex_lock(&mdl.proj_db_mutex);
}

void plt_unlock_projdb()
{
    pthread_mutex_unlock(&mdl.proj_db_mutex);
}

sqlite3 *plt_get_projdb()
{
    return mdl.proj_db;
}
extern int VERSION[3];

int plt_get_sw_ver()
{
    return VERSION[0];
}

int get_snow_id(char *buf)
{
    unsigned char out[16] = {0};
    if (snow_get_id_as_binary(out) == false)
    {
        log_dbg("unable to generate snowid as binary");
        return -1;
    }

    char buf_temp[33] = {0};
    sprintf(buf_temp, "%02x%02x%02x%02x%02x%02x%02x%02x",
            out[0], out[1], out[2], out[3],
            out[4], out[5], out[6], out[7]);
    strcat(buf, buf_temp);

    buf_temp[0] = '\0';
    sprintf(buf_temp, "%02x%02x%02x%02x%02x%02x",
            out[8], out[9], out[10], out[11],
            out[12], out[13]);
    strcat(buf, buf_temp);

    buf_temp[0] = '\0';
    sprintf(buf_temp, "%02x%02x", out[14], out[15]);
    strcat(buf, buf_temp);

    return 0;
}

int plt_init_stp1()
{
    int ret = 0;

    syslog(LOG_INFO, "%s, ++", __func__);

    // memset(&STA, 0, sizeof(struct station_t));
    // mdl.version[0] = VERSION[0];
    // mdl.version[1] = VERSION[1];
    // mdl.version[2] = VERSION[2];

    if (log_init() != 0)
    {
        ret = -1;
    }
    else if (0)
    { // shm_init() != 0
        ret = -1;
    }
    else if (0)
    { // cfg_db_init() != 0
        ret = -1;
    }
    else if (0)
    { // project_db_init() != 0
        ret = -1;
    }
    else if (0)
    { // meter_init() != 0
        ret = -1;
    }
    else if (0)
    { // ctn_init() != 0
        ret = -1;
    }

    syslog(LOG_INFO, "%s, --, ret:%d", __func__, ret);
    return ret;
}

int plt_init_stp2(int bmdl)
{
    int ret = 0;
    log_dbg("%s, ++", __func__);

    if (mbs_start_Enjoy100kW() != 0)
    { // chan_init() != 0
        ret = -1;
    }
    else if (0)
    { // tbmqtt_init() != 0
        ret = -2;
    }
    else if (mqtt_init() != 0)
    {
        ret = -3;
    }
    else if (0)
    { // mac_init() != 0
        ret = -4;
    }

    log_dbg("%s, --, ret:%d", __func__, ret);
    return ret;
}

int plt_devm_str2nbr(char *szdev_model)
{
    int devm = DEVM_INVALID;
    if (strcmp(szdev_model, "zh200") == 0)
    {
        devm = DEVM_ZH200;
    }
    else if (strcmp(szdev_model, "dtsd1352") == 0)
    {
        devm = DEVM_DTSD1352;
    }
    else if (strcmp(szdev_model, "dlt645") == 0)
    {
        devm = DEVM_DLT645;
    }
    else
    {
        devm = DEVM_INVALID;
    }

    return devm;
}

void plt_sm_errcode2str(char *buf, int err)
{
}

void plt_sm_statecode2str(char *buf, int stat)
{
    switch (stat)
    {
    case SMST_LAUNCH:
        strcpy(buf, "launch");
        break;

    case SMST_OFFLINE:
        strcpy(buf, "offline");
        break;

    case SMST_READY:
        strcpy(buf, "ready");
        break;

    case SMST_STDBY:
        strcpy(buf, "stdby");
        break;

    case SMST_DHG:
        strcpy(buf, "dhg");
        break;

    case SMST_CHG:
        strcpy(buf, "chg");
        break;

    case SMST_ERR:
        strcpy(buf, "err");
        break;

    default:
        strcpy(buf, "unknown");
        break;
    }
}
