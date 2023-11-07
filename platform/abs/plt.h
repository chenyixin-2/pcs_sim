#ifndef PLT_H
#define PLT_H

#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/if.h>
#include <linux/socket.h>
#include <linux/sockios.h>
#include <linux/types.h>
#include <math.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <sys/types.h>
#include <syslog.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

/* component */
#include "mbs.h"
#include "mb.h"
#include "mbctx.h"

#include "cJSON.h"
#include "sqlite3.h"
#include "modbus.h"
#include "MQTTClient.h"
#include "MQTTClientPersistence.h"
#include "zlog.h"
#include "uuid4.h"
#include "snowid.h"
#define SNOW_ID_BUF_SIZE 34

#include "log.h"
#include "misc.h"
#include "sm.h"
#include "comm.h"
#include "shm.h"

/* util */
#include "mqtt_ringbuffer.h"
#include "tbmqtt_ringbuffer.h"
#include "tbmqtt_cache.h"
#include "mqtt.h"
#include "tbmqtt.h"
#include "mqtt_cache.h"

/* device */
#include "chan_ringbuffer.h"
#include "chan.h"
#include "mac.h"

/* abs */
#include "cloud.h"
#include "tb.h"
#include "mbs.h"
#include "mdl.h"

#define CHAN_NBR_MAX 8

enum statemachine_state_t
{
    SMST_LAUNCH = 10,
    SMST_OFFLINE = 11,
    SMST_STDBY = 12,
    SMST_IDLE = 13,
    SMST_DHG = 14,
    SMST_CHG = 15,
    SMST_ERR = 16,
    SMST_STOP = 17,
    SMST_RUN = 18,
    SMST_READY = 19,
    SMST_OFFGRID = 20,
};

enum statemachine_err_t
{
    SMERR_NONE = 0,
    SMERR_NA,
};

enum cmd_t
{
    CMD_SM_DONE = 0,
    CMD_SM_OFFLINE,
    CMD_SM_STDBY,
    CMD_SM_IDLE,
    CMD_SM_DHG,
    CMD_SM_CHG,
    CMD_SM_ACK,
    CMD_SM_NACK,
    CMD_SM_STOP,
    CMD_SM_READY,
    CMD_SM_RUN,
    CMD_SM_OFFGRID,
    CMD_SM_LAUNCH,

    CMD_STA = 100,
    CMD_STA_FETCH,
    CMD_STA_SENDCMD,
    CMD_STA_SET_ACTIVEPSET,
    CMD_STA_SET_PMODE,

    CMD_CTN_SENDCMD = 200,
    CMD_CTN_SET_CMD,
    CMD_CTN_SET_ACTIVEPSET,
    // CMD_CTN_SET_STAHEARTBEAT,
    CMD_CTN_SET_BSYTICK,

    CMD_CHAN = 300,
    CMD_CHAN_RESET,
    CMD_CHAN_SET_DBG,
    CMD_CHAN_SET_EN,

    CMD_PCS = 400,

    CMD_PACK = 500,

    CMD_METER = 600,
    CMD_METER_SENDCMD,
    CMD_METER_SENDCMD2,
    CMD_METER_SET_COMACTIVE,
    CMD_METER_SET_COMACTIVP,
    CMD_METER_SET_PT_MDL,
    CMD_METER_SET_CT_MDL,
    CMD_METER_SET_PT,
    CMD_METER_SET_CT,

    CMD_AC = 700,

    CMD_MQTT = 800,
    CMD_MQTT_SENDCMD,
    CMD_MQTT_SENDKV,

    CMD_CLOUD = 900,
    CMD_CLOUD_SET_ESSSYS_ENINTVAL,

    CMD_EMS = 10000,
    CMD_EMS_SET_MODE,
    CMD_EMS_SET_POWREG_HIGHLOW,
    CMD_EMS_SET_PIDX,
    CMD_EMS_RESET_PCURV,
    CMD_EMS_DELPCURV,
    CMD_EMS_EDITPCURV,
    CMD_EMS_SAVEPCURV,
    CMD_EMS_LOADPCURV,
    CMD_EMS_FETCHDCURV,
    CMD_EMS_NEWDCURV,
    CMD_EMS_DELDCURV,
    CMD_EMS_EDITDCURV,
    CMD_EMS_SAVEDCURV,
    CMD_EMS_EDITCHGTIME,
    CMD_EMS_EDITDHGTIME,
    CMD_METER_SETCTLREG,
    CMD_MISC_YDL,
    CMD_ALARM_SET, //
};

enum device_models_t
{
    DEVM_INVALID = -1,
    DEVM_ZH200,
    DEVM_DTSD1352,
    DEVM_DLT645,
};

#define SHMBUFSIZE 1 * 1024 * 1024
#define SHMID 0x1234567A
#define SEM_SHMPING "STASEMSHMPING"
#define SEM_SHMPONG "STASEMSHMPONG"

enum pmode_t
{
    PMOD_SOC = 0,
    PMOD_AVE,
    PMOD_AH,
};

struct shmparam_t
{
    int bidx;
    int idx;
    int adr;
    int id;
    int val;
    int val2;
    float fval;
    double dval;
    int val_arr[8];
    double dval_arr[8];

    char szbuf[512];
    int szbuflen;
};

struct shm_t
{
    pthread_mutex_t mutex;
    int cmd;
    int rsp;
    struct shmparam_t param;
    unsigned char buf[SHMBUFSIZE];
};

enum sta_err_t
{
    STAERR_NONE = 0,
    STAERR_NA,
    STAERR_NACK,

    /* launch */
    STAERR_LAUNCH_WAIT_ALL_CTN_STDBY_TIMEOUT,
    STAERR_LAUNCH_WAIT_ALL_METER_READY_TIMEOUT,

    /* stdby */
    STAERR_STDBY_OFFLINE_CTN_DETECTED,
    STAERR_STDBY_NOT_READY_METER_DETECTED,
    STAERR_STDBY_WAIT_ALL_CTN_STOP_TIMEOUT,

    /* stop */
    STAERR_STOP_OFFLINE_CTN_DETECTED,
    STAERR_STOP_NOT_READY_METER_DETECTED,
    STAERR_STOP_WAIT_ALL_CTN_STDBY_TIMEOUT,
    STAERR_STOP_WAIT_ALL_CTN_READY_TIMEOUT,

    /* ready */
    STAERR_READY_NOT_READY_CTN_DETECTED,
    STAERR_READY_NOT_READY_METER_DETECTED,
    STAERR_READY_DISPOW_FAIL,
    STAERR_READY_WAIT_DHG_TIMEOUT,
    STAERR_READY_WAIT_CHG_TIMEOUT,
    STAERR_READY_WAIT_ALL_CTN_STOP_TIMEOUT,

    /* dhg */
    STAERR_DHG_DISPOW_FAIL,
    STAERR_DHG_WAIT_ALL_CTN_READY_TIMEOUT,
    STAERR_DHG_NOT_READY_METER_DETECTED,
    STAERR_DHG_ERR_CTN_DETECTED,
    STAERR_DHG_OFFLINE_CTN_DETECTED,

    /* chg */
    STAERR_CHG_DISPOW_FAIL,
    STAERR_CHG_WAIT_ALL_CTN_READY_TIMEOUT,
    STAERR_CHG_NOT_READY_METER_DETECTED,
    STAERR_CHG_ERR_CTN_DETECTED,
    STAERR_CHG_OFFLINE_CTN_DETECTED,
};


enum pow_dis_mode_t
{
    PDMOD_SOC = 0,
    PDMOD_AVG,
};

struct dbcbparam_t
{
    int idx;
    int ret;
    int nrow; // enabled row
};

struct mdl_t
{
    char szDevName[128];
    char szSerial[128];
    int adr;
    char szmqtt_servip[16];
    int mqtt_servport;

    double uab;
    double ubc;
    double uca;
    double ua;
    double ub;
    double uc;    
    double ia;
    double ib;
    double ic;
    double gf;  // grid frequency
    double ap;  // ac total active power
    double rap; // ac total inactive power
    double pf;
    double dcv;
    double posdcv;
    double negdcv;
    double batv;
    double bati;
    double dcp;
    unsigned short work_state;
    double igbt_temp;
    double env_temp;
    double ind_temp;
    unsigned short work_mode;

    bool brunning;

    char szinfo[64];
    char szprojId[128];
    int idx;

    int time_zone;

    char sztime[32];
    int version[3];

    sqlite3 *cfg_db;
    pthread_mutex_t cfg_db_mutex;

    sqlite3 *proj_db;
    pthread_mutex_t proj_db_mutex;

    int con_ap;
    int load_ap;

    // int PloadMode;
    // char szPloadMode[16];
    // int PconMode;
    // char szPconMode[16];

    struct power_t pow;
    struct statemachine_t sm;
    int cmd;

    int channbr;
    struct chan_t chan[CHAN_NBR_MAX + 1];

    struct mqtt_t mqtt;
    struct tbmqtt_t tbmqtt;

    struct cloud_t cloud;
    struct mac_t mac;
};

extern struct mdl_t MDL;

int plt_init_stp1();
int plt_init_stp2();
void plt_sm_statecode2str(char *buf, int stat);
void plt_sm_errcode2str(char *buf, int err);
int plt_devm_str2nbr(char *szdev_model);
void plt_lock_cfgdb();
void plt_unlock_cfgdb();
sqlite3 *plt_get_cfgdb();
int plt_init_projdb();
void plt_lock_projdb();
void plt_unlock_projdb();
sqlite3 *plt_get_projdb();
int plt_get_sw_ver();
int get_snow_id(char *buf);

#endif /* PLT_H */
