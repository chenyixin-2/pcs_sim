#include "plt.h"

static int appl_init()
{
    int ret = 0;

    log_dbg("%s, addr : %d, device name: %s, serial port: %s, mqtt srv ip: %s, mqtt srv port: %d  ++", __func__, MDL.adr, MDL.szDevName, MDL.szSerial, MDL.szmqtt_servip, MDL.mqtt_servport);
    if (mdl_init() != 0)
    { // sta_init() != 0
        ret = -1;
    }
    else if (cloud_init() != 0)
    { /* cloud_init() != 0 */
        ret = -1;
    }

    log_dbg("%s, -- ret:%d", __func__, ret);
    return ret;
}

static int appl_run()
{
    while (1)
    {
        usleep(90000);
        cloud_exe();
    }
}

extern int DAEMON;
int appl_main(void)
{
    int ret = 0;

    syslog(LOG_INFO, "%s, ++", __func__);

    if (DAEMON == 1)
    {
        if (misc_daemon(true, false) != 0)
        {
            log_dbg("%s, become daemon fail", __func__);
            ret = -1;
            goto leave;
        }
    }

    if (plt_init_stp1() != 0)
    {
        log_dbg("%s, plt_init_stp1 fail", __func__);
        ret = -1;
    }
    else if (appl_init() != 0)
    {
        log_dbg("%s, appl_init fail", __func__);
        ret = -1;
    }
    else if (plt_init_stp2() != 0)
    {
        log_dbg("%s, appl_init_stp2 fail", __func__);
        ret = -1;
    }
    else
    {
        appl_run();
    }

leave:
    syslog(LOG_INFO, "%s, --, ret:%d", __func__, ret);
    return ret;
}